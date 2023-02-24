/* -------------------- IVFGFold.cpp ------------------ */
//
// Created by kisslune on 2/23/23.
//

#include "VFA/IVFGFold.h"

using namespace SVF;


void IVFGCompact::compactGraph()
{
    /// detect compactable pairs
    for (auto edge: lg->getIVFGEdges()) {
        if (edge->getEdgeKind() != IVFG::DirectVF)
            continue;

        auto srcOutEdges = edge->getSrcNode()->getOutEdges();
        auto dstInEdges = edge->getDstNode()->getInEdges();
//        if (dstInEdges.size() <= 1 && !edge->getDstNode()->isSrc())   /// src info required
//            compactPairs.push(std::make_pair(edge->getSrcID(), edge->getDstID()));
        if (dstInEdges.size() <= 1)     /// src info not required
            compactPairs.push(std::make_pair(edge->getSrcID(), edge->getDstID()));
    }

    /// merge compactable pairs
    while (!compactPairs.empty()) {
        NodePair pair = compactPairs.top();
        compactPairs.pop();
        NodeID src = lg->repNodeID(pair.first);
        NodeID dst = lg->repNodeID(pair.second);
        if (src == dst)
            continue;

        lg->mergeNodeToRep(dst, src);
    }
}
