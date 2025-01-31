/*
 * CFLSolver.h
 *
 *  Created on: Nov 22, 2019
 *      Author: Yuxiang Lei
 */

#ifndef CFGDATA_H_
#define CFGDATA_H_

#include "Util/WorkList.h"

namespace SVF
{
typedef std::pair<char, u32_t> Label;

/*!
 * Adjacency-list graph representation
 */
class CFLData
{
public:
    typedef std::map<const Label, NodeBS> TypeMap;
    typedef std::unordered_map<NodeID, TypeMap> DataMap;
    typedef typename DataMap::iterator iterator;
    typedef typename DataMap::const_iterator const_iterator;

protected:
    DataMap succMap;
    DataMap predMap;
    const NodeBS emptyData;
    NodeBS diff;

    // union/add data
    //@{
    inline bool addPred(const NodeID key, const NodeID src, const Label ty)
    {
        return predMap[key][ty].test_and_set(src);
    };

    inline bool addSucc(const NodeID key, const NodeID dst, const Label ty)
    {
        return succMap[key][ty].test_and_set(dst);
    };

    inline bool addPreds(const NodeID key, const NodeBS& data, const Label ty)
    {
        if (data.empty())
            return false;
        return predMap[key][ty] |= data;
    }

    inline bool addSuccs(const NodeID key, const NodeBS& data, const Label ty)
    {
        if (data.empty())
            return false;
        return succMap[key][ty] |= data;
    }
    //@}

public:
    // Constructor
    CFLData()
    {}

    // Destructor
    virtual ~CFLData()
    {}

    virtual void clear()
    {
        succMap.clear();
        predMap.clear();
    }

    inline const_iterator begin() const
    {
        return succMap.begin();
    }

    inline const_iterator end() const
    {
        return succMap.end();
    }

    inline iterator begin()
    {
        return succMap.begin();
    }

    inline iterator end()
    {
        return succMap.end();
    }

    inline DataMap& getSuccMap()
    {
        return succMap;
    }

    inline DataMap& getPredMap()
    {
        return predMap;
    }

    inline TypeMap& getSuccMap(const NodeID key)
    {
        return succMap[key];
    }

    inline TypeMap& getPredMap(const NodeID key)
    {
        return predMap[key];
    }

    inline NodeBS& getSuccs(const NodeID key, const Label ty)
    {
        return succMap[key][ty];
    }

    inline NodeBS& getPreds(const NodeID key, const Label ty)
    {
        return predMap[key][ty];
    }

    // Alias data operations
    //@{
    inline bool addEdge(const NodeID src, const NodeID dst, const Label ty)
    {
        addSucc(src, dst, ty);
        return addPred(dst, src, ty);
    }

    inline NodeBS addEdges(const NodeID src, const NodeBS& dstData, const Label ty)
    {
        NodeBS newDsts;
        if (addSuccs(src, dstData, ty))
        {
            for (const NodeID datum: dstData)
                if (addPred(datum, src, ty))
                    newDsts.set(datum);
        }
        return newDsts;
    }

    inline NodeBS addEdges(const NodeBS& srcData, const NodeID dst, const Label ty)
    {
        NodeBS newSrcs;
        if (addPreds(dst, srcData, ty))
        {
            for (const NodeID datum: srcData)
                if (addSucc(datum, dst, ty))
                    newSrcs.set(datum);
        }
        return newSrcs;
    }

    inline bool hasEdge(const NodeID src, const NodeID dst, const Label ty)
    {
        const_iterator iter1 = succMap.find(src);
        if (iter1 == succMap.end())
            return false;

        auto iter2 = iter1->second.find(ty);
        if (iter2 == iter1->second.end())
            return false;

        return iter2->second.test(dst);
    }

    /* This is a dataset version, to be modified to a cflData version */
    inline void clearEdges(const NodeID key)
    {
        succMap[key].clear();
        predMap[key].clear();
    }
    //@}
};


/*!
 * Hybrid graph representation for transitive relations
 */
class HybridData
{
public:
    struct TreeNode
    {
        NodeID id;
        std::unordered_set<TreeNode*> children;

        TreeNode(NodeID nId) : id(nId)
        {}

        inline bool operator==(const TreeNode& rhs) const
        {
            return id == rhs.id;
        }

        inline bool operator<(const TreeNode& rhs) const
        {
            return id < rhs.id;
        }
    };


public:
    Map <NodeID, std::unordered_map<NodeID, TreeNode*>> indMap;   // indMap[v][u] points to node v in tree(u)

    HybridData()
    {}

    ~HybridData()
    {
        for (auto iter1: indMap)
        {
            for (auto iter2: iter1.second)
            {
                delete iter2.second;
                iter2.second = NULL;
            }
        }
    }

    bool hasInd(NodeID src, NodeID dst)
    {
        auto it = indMap.find(dst);
        if (it == indMap.end())
            return false;
        return (it->second.find(src) != it->second.end());
    }

    /// Add a node dst to tree(src)
    TreeNode* addInd(NodeID src, NodeID dst)
    {
        auto resIns = indMap[dst].insert(std::make_pair(src, new TreeNode(dst)));
        if (resIns.second)
            return resIns.first->second;
        return nullptr;
    }

    /// Get the node dst in tree(src)
    TreeNode* getNode(NodeID src, NodeID dst)
    {
        return indMap[dst][src];
    }

    /// add v into desc(x) as a child of u
    void insertEdge(TreeNode* u, TreeNode* v)
    {
        u->children.insert(v);
    }

    void addArc(NodeID src, NodeID dst)
    {
        if (!hasInd(src, dst))
        {
            for (auto iter: indMap[src])
            {
                meld(iter.first, getNode(iter.first, src), getNode(dst, dst));
            }
        }
    }

    void meld(NodeID x, TreeNode* uNode, TreeNode* vNode)
    {
        TreeNode* newVNode = addInd(x, vNode->id);
        if (!newVNode)
            return;

        insertEdge(uNode, newVNode);
        for (TreeNode* vChild: vNode->children)
        {
            meld(x, newVNode, vChild);
        }
    }
};


}   // end namespace SVF

#endif
