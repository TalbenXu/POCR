//
// Created by kisslune on 3/13/22.
//

#include "VFA/VFAnalysis.h"

using namespace SVF;


void GspanVFA::initSolver()
{
    for (CFLEdge* edge: graph()->getIVFGEdges()) {
        if (edge->getEdgeKind() == IVFG::DirectVF) {
            addEdge(edge->getSrcID(), edge->getDstID(), std::make_pair(a, 0));
        }
        else if (edge->getEdgeKind() == IVFG::CallVF) {
            u32_t offset = edge->getEdgeIdx();
            addEdge(edge->getSrcID(), edge->getDstID(), std::make_pair(call, offset));
        }
        else if (edge->getEdgeKind() == IVFG::RetVF) {
            u32_t offset = edge->getEdgeIdx();
            addEdge(edge->getSrcID(), edge->getDstID(), std::make_pair(ret, offset));
        }
    }

    for (auto nIter = graph()->begin(); nIter != graph()->end(); ++nIter) {
        NodeID nodeId = nIter->first;
        addEdge(nodeId, nodeId, std::make_pair(A, 0));
    }
}


void GspanVFA::solve()
{
    numOfIteration++;
    reanalyze = false;
    CFLData resData;

    for (auto& srcIter: cflData()->getSuccMap()) {
        NodeID src = srcIter.first;

        // old + new
        for (auto& tyIter: oldData()->getSuccMap(src)) {
            Label lty = tyIter.first;
            for (NodeID oldDst1: tyIter.second) {
                // new
                for (auto& tyIter2: cflData()->getSuccMap(oldDst1)) {
                    Label newTy = binarySumm(lty, tyIter2.first);
                    if (newTy.first && (resData.getSuccs(src, newTy) |= tyIter2.second))
                        checks += tyIter2.second.count();       // stat
                }
            }
        }

        // new + old and new
        for (auto tyIter: cflData()->getSuccMap(src)) {
            Label lty = tyIter.first;
            Label newTy = unarySumm(lty);
            for (NodeID newDst1: tyIter.second) {
                if (newTy.first && resData.getSuccs(src, newTy).test_and_set(newDst1))
                    checks++;       // stat
                // old
                for (auto& tyIter2: oldData()->getSuccMap(newDst1)) {
                    Label newTy = binarySumm(lty, tyIter2.first);
                    if (newTy.first && (resData.getSuccs(src, newTy) |= tyIter2.second))
                        checks += tyIter2.second.count();       // stat
                }
                // new
                for (auto& tyIter2: cflData()->getSuccMap(newDst1)) {
                    Label newTy = binarySumm(lty, tyIter2.first);
                    if (newTy.first && (resData.getSuccs(src, newTy) |= tyIter2.second))
                        checks += tyIter2.second.count();       // stat
                }
            }
        }

        // update old and new
        for (auto& tyIter: cflData()->getSuccMap(src)) {
            oldData()->getSuccs(src, tyIter.first) |= tyIter.second;
            tyIter.second.clear();
        }
        for (auto& tyIter: resData.getSuccMap(src)) {
            cflData()->getSuccs(src, tyIter.first) = tyIter.second;
            cflData()->getSuccs(src, tyIter.first).intersectWithComplement(oldData()->getSuccs(src, tyIter.first));
            if (!cflData()->getSuccs(src, tyIter.first).empty())
                reanalyze = true;
        }
    }
}


void GspanVFA::countSumEdges()
{
    numOfSumEdges = 0;
    numOfTEdges = 0;
    std::set<u32_t> s = {A,Cl};

    for (auto iter1 = oldData()->begin(); iter1 != oldData()->end(); ++iter1) {
        for (auto& iter2: iter1->second) {
            if (s.find(iter2.first.first) != s.end())
                numOfSumEdges += iter2.second.count();
//            if (s1.find(iter2.first.first) != s1.end())
//                numOfTEdges += iter2.second.count();
        }
    }
}
