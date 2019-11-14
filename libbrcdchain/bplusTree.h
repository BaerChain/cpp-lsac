//
// Created by friday on 2019/10/24.
//

#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <iostream>
#include <libdevcore/Common.h>

#define TODO_FUNC { std::cout << "to do " << __LINE__ << __FILE__ << std::endl; assert(false); exit(1);}

#define HAS_MEMBER(member)\
template<typename T, typename... Args>struct has_member_##member\
{\
private:\
    template<typename U> static auto Check(int) -> decltype(std::declval<U>().member(std::declval<Args>()...), std::true_type()); \
    template<typename U> static auto Check(...) -> decltype(std::false_type()); \
public:\
static const bool value = std::is_same<decltype(Check<T>(0)), std::true_type>::value; \
}; \


namespace dev {

    namespace brc {
        typedef std::string NodeKey;

        typedef std::string DataKey;
        typedef bytes DataPackage;


        struct databaseDelegate {
            virtual DataPackage getData(const DataKey &nk) = 0;

            virtual void setData(const DataKey &nk, const DataPackage &dp) = 0;

            virtual void deleteKey(const DataKey &nk) = 0;
        };


        struct packedDataInterface {
            virtual void decode(const RLP &rlp) = 0;

            virtual bytes encode() = 0;
        };

        HAS_MEMBER(getData);

        HAS_MEMBER(setData);

        HAS_MEMBER(deleteKey);

        HAS_MEMBER(decode);

        HAS_MEMBER(encode);


        namespace internal {

            typedef std::pair<bool, size_t> find_ret_type;


            /// find key of vector pos.
            /// \tparam T
            /// \tparam Compare
            /// \param key
            /// \param m_values
            /// \param m_compare
            /// \return first insert value index , @bool  true : this value is existed.    false cant find value.
            template<typename T, typename Compare>
            find_ret_type findKeyIndex(const T &key, const std::vector<T> &values, Compare m_compare) {
                size_t front = 0;
                size_t end = values.size() - 1;
                size_t mid = (front + end) / 2;
                bool same = false;

                if (values.size() == 0) {
                    return {false, 0};
                }

                while (front < end) {
                    if (m_compare(key, values[mid])) {
                        end = mid > 0 ? mid - 1 : mid;
                    } else if (m_compare(values[mid], key)) {
                        front = mid + 1;
                    } else {
                        same = true;
                        break;
                    }
                    mid = (front + end) / 2;
                }

                if (m_compare(key, values[mid])) {

                } else if (m_compare(values[mid], key)) {
                    mid++;
                } else {
                    same = true;
                }
                return {same, mid};
            }

            template<typename T>
            void insertToVector(const T &key, std::vector<T> &values, size_t pos) {
                values.push_back(key);
                for (size_t i = values.size() - 1; i > pos; i--) {
                    std::swap(values[i], values[i - 1]);
                }
            }


            template<typename T>
            struct call_decode {
                 T call(const RLP &rlp){
                     T t;
                     t.decode(rlp);
                     return t;
                 };
            };

            template <>
            struct call_decode<uint32_t> {
                uint32_t call(const RLP &rlp){
                    return rlp.toInt<uint32_t>(0);
                };
            };

            template <>
            struct call_decode<std::string> {
                std::string call(const RLP &rlp){
                    return rlp.toString();
                };
            };


            template<typename T>
            struct call_encode{
                void call(const T &t, dev::RLPStream &rlp){
                    t.encode(rlp);
                }
            };

            template <>
            struct call_encode<uint32_t>{
                void call(const uint32_t &t, dev::RLPStream &rlp){
                    rlp.append(t);
                }
            };

            template <>
            struct call_encode<std::string>{
                void call(const std::string &t, dev::RLPStream &rlp){
                    rlp.append(t);
                }
            };




            template<typename KEY, typename VALUE, size_t L = 1024, typename Compare = std::less<KEY>>
            struct leaf  {

                typedef KEY key_type;
                typedef VALUE value_type;
                typedef std::pair<key_type, value_type> kv_pair;


                NodeKey mSelfKey;
                NodeKey mParentKey;
                std::vector<kv_pair> mValues;
                Compare mCompare;

                leaf() {}

                bytes encode() {
                    dev::RLPStream rlp(3);
                    rlp << mSelfKey;
                    rlp << mParentKey;
                    rlp.appendList(mValues.size());
                    for (auto &itr : mValues) {
                        rlp.appendList(2);
                        call_encode<key_type>().call(itr.first, rlp);
                        call_encode<value_type>().call(itr.second, rlp);

                    }
                    return rlp.out();
                }

                void decode(const bytes &data) {
                    RLP rlp(data);
                    if (rlp.itemCount() == 3) {
                        mSelfKey = rlp[0].convert<NodeKey>(RLP::LaissezFaire);
                        mParentKey = rlp[1].convert<NodeKey>(RLP::LaissezFaire);
                        mValues.reserve(rlp[2].itemCount());
                        for (const auto &itr : rlp[2]) {
                            auto k = call_decode<key_type>().call(itr[0]);
                            auto v = call_decode<value_type>().call(itr[1]);
                            mValues.push_back(kv_pair(k, v));
                        }
                    }

                }

                find_ret_type getIndex(const kv_pair &kv) {
                    auto com = [&](const kv_pair &v1, const kv_pair &v2) -> bool {
                        return mCompare(v1.first, v2.first);
                    };
                    return findKeyIndex(kv, mValues, com);
                }

                bool insertValue(const kv_pair &kv) {
                    return insertValue(kv.first, kv.second);
                }

                bool insertValue(const key_type &key, const value_type &value) {
                    find_ret_type index = getIndex(kv_pair(key, value));
                    if (!index.first) {
                        insertToVector(kv_pair(key, value), mValues, index.second);
                    } else {
                        mValues[index.second] = kv_pair(key, value);
                    }
                    return index.first;
                }

                bool isNull() {
                    return mSelfKey.empty() && mParentKey.empty() && !mValues.size();
                }

                bool removeValue(const key_type &key) {
                    kv_pair kp;
                    kp.first = key;
                    find_ret_type id = getIndex(kp);
                    if (id.first) {
                        mValues.erase(mValues.begin() + id.second);

                        return true;
                    }
                    return false;
                }
            };


            template<typename KEY, size_t L = 1024, typename Compare = std::less<KEY>>
            struct node  {

                typedef KEY key_type;


                NodeKey mSelfKey;
                NodeKey mParentKey;
                std::vector<key_type> mKeys;              //keys
                std::vector<NodeKey> mChildrenNodes;     //children keys.
                Compare mCompare;

                bytes encode() {
                    dev::RLPStream rlp(4);
                    rlp << mSelfKey;
                    rlp << mParentKey;
                    rlp.appendList(mKeys.size());
                    for (auto const &i: mKeys) {
                        call_encode<key_type>().call(i, rlp);
                    }

                    rlp << mChildrenNodes;

                    return rlp.out();
                }

                void decode(const bytes &data) {
                    RLP rlp(data);
                    try {
                        if (rlp.itemCount() == 4) {
                            mSelfKey = rlp[0].convert<NodeKey>(RLP::LaissezFaire);
                            mParentKey = rlp[1].convert<NodeKey>(RLP::LaissezFaire);
                            mKeys.reserve(rlp[2].itemCount());
                            for (const auto &itr : rlp[2]) {
                                auto v = call_decode<key_type>().call(itr);
                                mKeys.push_back(v);
                            }
                            mChildrenNodes = rlp[3].convert<std::vector<NodeKey>>(RLP::LaissezFaire);
                        }
                    } catch (const std::exception &e) {
                        cwarn << "exception tree : " << e.what() << std::endl;
                    }


                }


                void insertKey(const key_type &k, const NodeKey &chKey) {
                    auto id = findKeyIndex(k, mKeys, mCompare);
                    if (!id.first) {
                        insertToVector(k, mKeys, id.second);
                        insertToVector(chKey, mChildrenNodes, id.second + 1);
                    } else {
                        mChildrenNodes[id.second + 1] = chKey;
                    }

                }

                ///
                /// \param key
                /// \return
                NodeKey getChildrenNode(const key_type &key) {
//                assert(mKeys.size() > 0);
                    auto ret = findKeyIndex(key, mKeys, mCompare);
                    if (!ret.first) { //
                        return mChildrenNodes[ret.second];
                    }
                    return mChildrenNodes[ret.second + 1];

                }

                bool isNull() {
                    return mSelfKey.empty() && mParentKey.empty() && mKeys.empty() && mChildrenNodes.empty();
                }

                size_t getKeyIndex(const NodeKey &key) {
                    size_t index = 0;
                    for (auto &itr : mChildrenNodes) {
                        if (itr == key) {
                            break;
                        }
                        index++;
                    }
                    return index;
                }

                void removeKeyValue(const size_t &indexKey, const size_t &indexValue) {
                    assert(indexKey < mKeys.size() && indexValue < mChildrenNodes.size());
                    mKeys.erase(mKeys.begin() + indexKey);
                    mChildrenNodes.erase(mChildrenNodes.begin() + indexValue);
                }


            };


        }

        template<typename T>
        struct string_debug {
            std::string to_string(const T &t) {
                return t.to_string();
            }
        };

        template<>
        struct string_debug<uint32_t> {
            std::string to_string(const uint32_t &t) {
                return std::to_string(t);
            }
        };


        template<typename KEY, typename VALUE, size_t LENGTH = 1024, typename Compare = std::less<KEY>>
        struct bplusTree {
            static_assert(LENGTH > 2, "L must > 2");

            typedef KEY key_type;
            typedef VALUE value_type;
            typedef internal::node<key_type, LENGTH, Compare> node_type;
            typedef internal::leaf<key_type, VALUE, LENGTH, Compare> leaf_type;
            typedef std::pair<bool, node_type> node_result;
            typedef std::pair<bool, leaf_type> leaf_result;


//            static_assert(
//                    std::is_same<key_type, std::string>::value ||
//                    std::is_same<key_type, unsigned>::value ||
//                    std::is_same<key_type, dev::u256>::value
////                    || (has_member_encode<key_type>::value && has_member_decode<key_type>::value)
//                    ,"key must std::string , u256 or has member endcode and decode"
//            );

//            static_assert(
//                    std::is_same<value_type, std::string>::value ||
//                    std::is_same<key_type, unsigned>::value ||
//                    std::is_same<value_type, dev::u256>::value
////                    ||  (has_member_encode<value_type>::value && has_member_decode<value_type>::value)
//                    ,"value must std::string , u256 or has member endcode and decode"
//            );

            enum class NodeLeaf {
                node = 0,
                leaf = 1,
                null
            };


            bplusTree(std::shared_ptr<databaseDelegate> dl = nullptr) {
                mDelegate = dl;
                if(mDelegate){
                    auto rootKey = mDelegate->getData("rootKey");
                    if (rootKey.size()) {
                        setRootKey(rootKey);
                    }
                }
            }

            bool insert(const key_type &key, const value_type &value) {
                return __insert(key, value);
            }

            bool remove(const key_type &key) {
                return __remove(key);
            }




            void debug() {
                std::string out;
                _debug(rootKey, 0, out);
                std::cout << "*******************root key : " << rootKey << " *******************begin " << std::endl;
                std::cout << out << std::endl;
                std::cout << "*******************root key : " << rootKey << " *******************end " << std::endl;
            }

            void update() {
                if (mDelegate) {
                    for (auto &itr : mNodes) {
                        if (!itr.first.empty()) {
                            mDelegate->setData(itr.first, itr.second.encode());
                        }
                    }
                    for (auto &itr : mLeafs) {
                        if (!itr.first.empty()) {
                            mDelegate->setData(itr.first, itr.second.encode());
                        }
                    }
                    RLPStream rlp(1);
                    rlp << rootKey;
                    mDelegate->setData("rootKey", rlp.out());
                }
            };


        private:

            void setRootKey(const bytes &nd) {
                RLP rlp(nd);
                rootKey = rlp[0].convert<NodeKey>(RLP::LaissezFaire);
            }

            void _debug(const NodeKey &nd, size_t depth, std::string &ret) {
                auto type = getType(nd);
                if (type.first) {
                    if (type.second == NodeLeaf::node) {
                        auto node = getData(nd, mNodes);
                        for (size_t i = 0; i < depth; i++) {
                            ret += "\t";
                        }
                        ret += "key self: " + node.second.mSelfKey + "<p:" + node.second.mParentKey + ">" + " = ";
                        for (auto &itr : node.second.mKeys) {
                            ret += "," + string_debug<KEY>().to_string(itr);
                        }
                        ret += "\n";
                        for (auto &itr : node.second.mChildrenNodes) {
                            _debug(itr, depth + 1, ret);
                        }

                    } else if (type.second == NodeLeaf::leaf) {
                        auto leaf = getData(nd, mLeafs);
                        for (size_t i = 0; i < depth; i++) {
                            ret += "\t";
                        }
                        ret += "key self: " + leaf.second.mSelfKey + "<p:" + leaf.second.mParentKey + ">" + " = ";
                        for (auto &itr : leaf.second.mValues) {
                            ret += "," + string_debug<key_type>().to_string(itr.first);;
                        }
                        ret += "\n";
                    }
                } else {
                    assert(false);
                }
            }

            bool __remove(const key_type &key) {
                std::pair<key_type, value_type> find;
                find.first = key;
                auto ret = findInsertPos(find.first, find.second);
                if (ret.first) {
                    return false;
                }

                ret.second.removeValue(key);
                checkFormat(ret.second.mSelfKey);


                return true;
            }

            bool __insert(const key_type &key, const value_type &value) {
                NodeKey insertKey;
                bool insert_ret = false;
                auto root_type = getType(rootKey);

                if (root_type.second == NodeLeaf::null) {    //create root.
                    auto ret = createData(mLeafs);
                    insert_ret = ret.second.insertValue(key, value);
                    rootKey = ret.second.mSelfKey;
                    return true;
                } else if (root_type.second == NodeLeaf::leaf) {  //insert to root leaf.
                    auto &ret = getData(rootKey, mLeafs).second;
                    insert_ret = ret.insertValue(key, value);
                    insertKey = ret.mSelfKey;
                } else { // root node is not leaf,
                    auto ret = findInsertPos(key, value);
                    insert_ret = ret.second.insertValue(key, value);
                    insertKey = ret.second.mSelfKey;
                }

                checkFormat(insertKey);


                return insert_ret;
            }

            /// check node or leaf is format of tree.
            /// \param insertNk insert key.
            void checkFormat(const NodeKey &insertNk) {
                NodeKey checkNode = insertNk;

                while (checkNode.size() > 0) {
//                std::cout << "checkFormat <<<<<<<<<<<< \n";
//                debug();
                    switch (getType(checkNode).second) {
                        case NodeLeaf::node: {
                            checkNode = checkFormatNode(checkNode);
                            break;
                        }
                        case NodeLeaf::leaf: {
                            checkNode = checkFormatLeaf(checkNode);
                            break;
                        }
                        default: {
                            assert(false);
                        }

                    }
                }
            }


            NodeKey checkFormatNode(const NodeKey &nk) {
                if (!mNodes.count(nk)) {
                    return NodeKey();
                }

                auto &nd = getData(nk, mNodes).second;
                assert(!nd.isNull());
                if (nd.mKeys.size() == LENGTH + 1) {
                    size_t mid_pos = nd.mKeys.size() / 2;
                    auto &nodeLeft = createData(mNodes).second;
                    if (nd.mParentKey.empty()) {
                        auto &parent = createData(mNodes).second;

                        ///relative key of parent
                        nodeLeft.mParentKey = parent.mSelfKey;
                        nd.mParentKey = parent.mSelfKey;

                        ///relative key children.
                        parent.mChildrenNodes.push_back(nd.mSelfKey);
                        parent.mChildrenNodes.push_back(nodeLeft.mSelfKey);

                        ///values
                        parent.mKeys.push_back(nd.mKeys[mid_pos]);

                        nodeLeft.mKeys.resize(mid_pos);
                        std::copy(nd.mKeys.begin() + mid_pos + 1, nd.mKeys.end(), nodeLeft.mKeys.begin());

                        for (size_t i = mid_pos + 1; i < nd.mChildrenNodes.size(); i++) {
                            auto pb = nd.mChildrenNodes[i];
                            switch (getType(pb).second) {
                                case NodeLeaf::node : {
                                    getData(pb, mNodes).second.mParentKey = nodeLeft.mSelfKey;
                                    break;
                                }
                                case NodeLeaf::leaf : {
                                    getData(pb, mLeafs).second.mParentKey = nodeLeft.mSelfKey;
                                    break;
                                }
                                default: {
                                    assert(false);
                                }
                            }
                            nodeLeft.mChildrenNodes.push_back(pb);
                        }


                        ///pop value.
                        size_t pop_size = mid_pos + 1;
                        while (pop_size-- != 0) {
                            nd.mKeys.pop_back();
                            nd.mChildrenNodes.pop_back();
                        }
                        rootKey = parent.mSelfKey;
                        return parent.mSelfKey;
                    } else {
                        nodeLeft.mKeys.resize(mid_pos);
                        std::copy(nd.mKeys.begin() + mid_pos + 1, nd.mKeys.end(), nodeLeft.mKeys.begin());

                        for (size_t i = mid_pos + 1; i < nd.mChildrenNodes.size(); i++) {
                            modifyParentByNodeKey(nd.mChildrenNodes[i], nodeLeft.mSelfKey);
                            nodeLeft.mChildrenNodes.push_back(nd.mChildrenNodes[i]);
                        }

                        ///pop value.
                        size_t pop_size = mid_pos + 1;
                        while (pop_size-- != 0) {
                            nd.mKeys.pop_back();
                            nd.mChildrenNodes.pop_back();
                        }

                        auto &parentNode = getData(nd.mParentKey, mNodes).second;
                        parentNode.insertKey(nd.mKeys[mid_pos], nodeLeft.mSelfKey);
                        nodeLeft.mParentKey = parentNode.mSelfKey;
                        assert(parentNode.mSelfKey == nodeLeft.mParentKey &&
                               nodeLeft.mParentKey == nodeLeft.mParentKey);
                        return parentNode.mSelfKey;
                    }
                } else if (nd.mKeys.size() < LENGTH / 2) {
                    if (nd.mParentKey.empty()) {
                        return NodeKey();
                    }

                    return catchValueFromBrother(nd.mSelfKey);
                }

                return NodeKey();
            }

            NodeKey checkFormatLeaf(const NodeKey &nk) {
                auto &nd = getData(nk, mLeafs).second;
                assert(!nd.isNull());
                if (nd.mValues.size() == LENGTH + 1) {
                    size_t mid_pos = nd.mValues.size() / 2;

                    auto &leaf = createData(mLeafs).second;
                    if (nd.mParentKey.empty()) {
                        auto &node = createData(mNodes).second;

                        //relative key of parent
                        nd.mParentKey = node.mSelfKey;
                        leaf.mParentKey = node.mSelfKey;

                        //relative key children.
                        node.mChildrenNodes.push_back(nd.mSelfKey);
                        node.mChildrenNodes.push_back(leaf.mSelfKey);

                        //value
                        node.mKeys.push_back(nd.mValues[mid_pos].first);

                        leaf.mValues.resize(mid_pos + 1);
                        std::copy(nd.mValues.begin() + mid_pos, nd.mValues.end(), leaf.mValues.begin());

                        size_t pop_size = mid_pos + 1;
                        while (pop_size-- != 0) {
                            nd.mValues.pop_back();
                        }

                        rootKey = node.mSelfKey;
                        return node.mSelfKey;
                    } else {
                        leaf.mValues.resize(mid_pos + 1);
                        std::copy(nd.mValues.begin() + mid_pos, nd.mValues.end(), leaf.mValues.begin());

                        size_t pop_size = mid_pos + 1;
                        while (pop_size-- != 0) {
                            nd.mValues.pop_back();
                        }

                        auto &parentNode = getData(nd.mParentKey, mNodes).second;
                        parentNode.insertKey(nd.mValues[mid_pos].first, leaf.mSelfKey);
                        leaf.mParentKey = nd.mParentKey;
                        assert(parentNode.mSelfKey == leaf.mParentKey && parentNode.mSelfKey == leaf.mParentKey);
                        return parentNode.mSelfKey;
                    }
                } else if (nd.mValues.size() < LENGTH / 2) {
                    if (nd.mParentKey.empty()) {
                        return NodeKey();
                    }

                    return catchValueFromBrother(nd.mSelfKey);
                }

                return NodeKey();
            }


            bool moveValueFromTo(node_type &from, node_type &to) {
                assert(!from.isNull() && !to.isNull() && from.mParentKey == to.mParentKey);
                auto &parent = getData(from.mParentKey, mNodes).second;
                size_t indexFrom = getIndexInParent(from.mSelfKey);
                size_t indexTo = getIndexInParent(to.mSelfKey);

                if (indexFrom > indexTo) {
                    to.insertKey(parent.mKeys[indexTo], from.mChildrenNodes[0]);
                    parent.mKeys[indexTo] = from.mKeys[0];
                    modifyParentByNodeKey(from.mChildrenNodes[0], to.mSelfKey);
                    from.removeKeyValue(0, 0);

                } else if (indexTo > indexFrom) {
                    parent.mKeys[indexFrom] = from.mKeys.back();

                    to.insertKey(parent.mKeys[indexFrom], from.mChildrenNodes.back());

                    from.removeKeyValue(from.mKeys.size() - 1, from.mChildrenNodes.size() - 1);

                    modifyParentByNodeKey(from.mChildrenNodes.back(), to.mSelfKey);
                } else {
                    assert(false);
                }


                return false;
            }

            bool moveValueFromTo(leaf_type &from, leaf_type &to) {
                assert(!from.isNull() && !to.isNull() && from.mParentKey == to.mParentKey);
                auto &parent = getData(from.mParentKey, mNodes).second;
                size_t indexFrom = getIndexInParent(from.mSelfKey);
                size_t indexTo = getIndexInParent(to.mSelfKey);

                if (indexFrom > indexTo) {
                    to.mValues.push_back(from.mValues[0]);
                    from.mValues.erase(from.mValues.begin());
                    parent.mKeys[indexTo] = from.mValues[0].first;
                    return true;
                } else if (indexTo > indexFrom) {
                    to.insertValue(from.mValues.back());
                    parent.mKeys[indexFrom] = from.mValues.back().first;
                    from.mValues.pop_back();
                    return true;
                } else {
                    assert(false);
                }

                return false;
            }

            bool moveAllValueTo(leaf_type &from, leaf_type &to) {
                size_t indexFrom = getIndexInParent(from.mSelfKey);

                auto &parent = getData(from.mParentKey, mNodes).second;
                for (size_t i = 0; i < from.mValues.size(); i++) {
                    to.insertValue(from.mValues[i]);
                }

                size_t remove_key = indexFrom > 0 ? indexFrom - 1 : 0;
                parent.mKeys.erase(parent.mKeys.begin() + remove_key);
                parent.mChildrenNodes.erase(parent.mChildrenNodes.begin() + indexFrom);
                deleteData(mLeafs, from.mSelfKey);
                return true;
            }

            bool moveAllValueTo(node_type &from, node_type &to) {
                size_t indexFrom = getIndexInParent(from.mSelfKey);
                auto &parent = getData(from.mParentKey, mNodes).second;
                if (parent.mKeys.size() == 1) {
                    from.mKeys.push_back(parent.mKeys.back());
                    for (size_t i = 0; i < to.mKeys.size(); i++) {
                        from.mKeys.push_back(to.mKeys[i]);

                    }
                    for (size_t i = 0; i < to.mChildrenNodes.size(); i++) {
                        from.mChildrenNodes.push_back(to.mChildrenNodes[i]);
                        modifyParentByNodeKey(from.mChildrenNodes.back(), from.mSelfKey);
                    }

                    deleteData(mNodes, parent.mSelfKey);
                    deleteData(mNodes, to.mSelfKey);
                    from.mParentKey.clear();
                    rootKey = from.mSelfKey;
                    return false;
                } else {
                    size_t indexTo = getIndexInParent(to.mSelfKey);

                    if (indexFrom > indexTo) {
                        //catch key from parent.
                        to.insertKey(parent.mKeys[indexTo], from.mChildrenNodes.front());



                        ///modify parent .
                        modifyParentByNodeKey(to.mChildrenNodes.front(), to.mSelfKey);

                        //merge value from to.
                        for (size_t i = 0; i < from.mKeys.size(); i++) {
                            to.insertKey(from.mKeys[i + 1], from.mChildrenNodes[i + 1]);
                            modifyParentByNodeKey(from.mChildrenNodes[i], to.mSelfKey);
                        }
                        //remove parent key and child.
                        parent.removeKeyValue(indexFrom - 1, indexFrom);
                        //remove from.
                        deleteData(mNodes, from.mSelfKey);

                    } else if (indexTo > indexFrom) {
                        //copy keys and values.
                        to.mKeys.insert(to.mKeys.begin(), parent.mKeys[indexFrom]);
                        to.mChildrenNodes.insert(to.mChildrenNodes.begin(), from.mChildrenNodes.back());

                        modifyParentByNodeKey(from.mChildrenNodes.back(), to.mSelfKey);
                        for (size_t i = 0; i < from.mKeys.size(); i++) {
                            to.mKeys.insert(to.mKeys.begin(), from.mKeys[i]);
                            to.mChildrenNodes.insert(to.mChildrenNodes.begin(), from.mChildrenNodes[i]);
                            modifyParentByNodeKey(from.mChildrenNodes[i], to.mSelfKey);
                        }
                        parent.removeKeyValue(indexFrom, indexFrom);
                        deleteData(mNodes, from.mSelfKey);
                    } else {
                        return false;
                    }

                }

                return true;
            }

            bool modifyParentByNodeKey(const NodeKey &nk, const NodeKey &parent) {
                auto type = getType(nk);
                if (type.first) {
                    if (type.second == NodeLeaf::leaf) {
                        auto leaf = getData(nk, mLeafs);
                        assert(leaf.first);
                        leaf.second.mParentKey = parent;
                    } else if (type.second == NodeLeaf::node) {
                        auto node = getData(nk, mNodes);
                        assert(node.first);
                        node.second.mParentKey = parent;
                    } else {
                        assert(false);
                    }

                    return true;
                }
                return false;
            }


            /// catch value from brother,
            /// \param nk
            /// \return
            NodeKey catchValueFromBrother(const NodeKey &nk) {

                auto type = getType(nk);
                if (type.first) {
                    switch (type.second) {
                        case NodeLeaf::leaf : {
                            auto &nd = getData(nk, mLeafs).second;
                            auto &parent = getData(nd.mParentKey, mNodes).second;
                            size_t indexOf = getIndexInParent(nd.mSelfKey);

                            if (indexOf == 0) {               ///from right.
                                auto &rightLeaf = getData(parent.mChildrenNodes[indexOf + 1], mLeafs).second;
                                if (rightLeaf.mValues.size() > LENGTH / 2) {
                                    moveValueFromTo(rightLeaf, nd);
                                } else {
                                    moveAllValueTo(nd, rightLeaf);
                                }
                            } else if (indexOf + 1 == parent.mChildrenNodes.size()) { ///from left
                                auto &leftLeaf = getData(parent.mChildrenNodes[indexOf - 1], mLeafs).second;
                                if (leftLeaf.mValues.size() > LENGTH / 2) {
                                    moveValueFromTo(leftLeaf, nd);
                                } else {
                                    moveAllValueTo(nd, leftLeaf);
                                }
                            } else {
                                if (getData(parent.mChildrenNodes[indexOf + 1], mLeafs).second.mValues.size() >
                                    LENGTH / 2) {
                                    ///from right.
                                    moveValueFromTo(getData(parent.mChildrenNodes[indexOf + 1], mLeafs).second, nd);
                                } else if (getData(parent.mChildrenNodes[indexOf - 1], mLeafs).second.mValues.size() >
                                           LENGTH / 2) {
                                    ///from left
                                    moveValueFromTo(getData(parent.mChildrenNodes[indexOf - 1], mLeafs).second, nd);
                                } else {
                                    moveAllValueTo(nd, getData(parent.mChildrenNodes[indexOf - 1], mLeafs).second);
                                }
                            }
                            return parent.mSelfKey;
                        }
                        case NodeLeaf::node : {
                            auto &nd = getData(nk, mNodes).second;
                            auto &parent = getData(nd.mParentKey, mNodes).second;
                            size_t indexOf = getIndexInParent(nd.mSelfKey);

                            if (indexOf == 0) {               ///from right.
                                auto &rightNode = getData(parent.mChildrenNodes[indexOf + 1], mNodes).second;
                                if (rightNode.mKeys.size() > LENGTH / 2) {
                                    moveValueFromTo(rightNode, nd);
                                } else {
                                    if (!moveAllValueTo(nd, rightNode)) {
                                        return NodeKey();
                                    }
                                }
                            } else if (indexOf + 1 == parent.mChildrenNodes.size()) { ///from left
                                auto &leftLeaf = getData(parent.mChildrenNodes[indexOf - 1], mNodes).second;
                                if (leftLeaf.mKeys.size() > LENGTH / 2) {
                                    moveValueFromTo(nd, leftLeaf);
                                } else {
                                    moveAllValueTo(nd, leftLeaf);
                                }
                            } else {
                                if (getData(parent.mChildrenNodes[indexOf + 1], mNodes).second.mKeys.size() >
                                    LENGTH / 2) {
                                    ///from right.
                                    moveValueFromTo(getData(parent.mChildrenNodes[indexOf + 1], mNodes).second, nd);
                                } else if (getData(parent.mChildrenNodes[indexOf - 1], mNodes).second.mKeys.size() >
                                           LENGTH / 2) {
                                    ///from left
                                    moveValueFromTo(nd, getData(parent.mChildrenNodes[indexOf - 1], mNodes).second);
                                } else {
                                    moveAllValueTo(nd, getData(parent.mChildrenNodes[indexOf + 1], mNodes).second);
                                }
                            }
                            return parent.mSelfKey;
                        }
                        default: {
                            return NodeKey();
                        }
                    }
                }
                return NodeKey();
            }


            size_t getIndexInParent(const NodeKey &nk) {
                auto type = getType(nk);
                if (type.first) {
                    if (type.second == NodeLeaf::node) {
                        auto &nd = getData(nk, mNodes).second;
                        if (!nd.mParentKey.empty()) {
                            auto &parent = getData(nd.mParentKey, mNodes).second;
                            return parent.getKeyIndex(nd.mSelfKey);
                        }
                    } else if (type.second == NodeLeaf::leaf) {
                        auto &nd = getData(nk, mLeafs).second;
                        if (!nd.mParentKey.empty()) {
                            auto &parent = getData(nd.mParentKey, mNodes).second;
                            return parent.getKeyIndex(nd.mSelfKey);
                        }
                    }
                }

                return 0;
            }

            ///
            /// \param key
            /// \param value
            /// \return
            std::pair<bool, leaf_type &> findInsertPos(const key_type &key, const value_type &value) {
                NodeKey findKey = rootKey;
                while (true) {
                    auto find_node_type = getType(findKey);
                    assert(find_node_type.first);

                    if (find_node_type.second == NodeLeaf::node) {
                        auto ret = getData(findKey, mNodes);
                        assert(ret.first);
                        findKey = ret.second.getChildrenNode(key);
                        continue;
                    } else if (find_node_type.second == NodeLeaf::leaf) {
                        auto ret = getData(findKey, mLeafs);
                        assert(ret.first);
                        findKey = ret.second.mSelfKey;
                        break;
                    } else {
                        assert(false);
                    }
                }

                return {false, getData(findKey, mLeafs).second};
            }

            /// get node type by key
            /// \param nk nodekey
            /// \return  @bool true : find , false not find , @NodeLeaf node type.
            std::pair<bool, NodeLeaf> getType(const NodeKey &nk) {
                if (nk.size() == 0) {
                    return {false, NodeLeaf::null};
                }

                if (getData(nk, mLeafs).first) {
                    return {true, NodeLeaf::leaf};
                }

                if (getData(nk, mNodes).first) {
                    return {true, NodeLeaf::node};
                }

                return {false, NodeLeaf::null};
            }


            NodeKey generateKey() {
                return std::to_string(mGenerateKey++);
            }

            ////////////////////
            template<typename T>
            std::pair<bool, T &> getData(const NodeKey &nk, std::map<NodeKey, T> &db) {
                if (db.count(nk)) {
                    return {true, db[nk]};
                }
                if (mDelegate) {
                    auto data = mDelegate->getData(nk);
                    if (data.size()) {
                        T t;
                        t.decode(data);
                        if (t.isNull()) {
                            return {false, db[NodeKey()]};
                        }
                        db[nk] = t;
                        return {true, db[nk]};
                    }
                }
                return {false, db[NodeKey()]};
            }


            template<typename T>
            std::pair<bool, T &> createData(std::map<NodeKey, T> &db) {
                T t;
                t.mSelfKey = generateKey();
                db[t.mSelfKey] = t;
                return {true, db[t.mSelfKey]};
            }

            template<typename T>
            bool deleteData(std::map<NodeKey, T> &db, const NodeKey &nd) {
                db.erase(nd);
                return true;
            }


            NodeKey rootKey;
            uint64_t mGenerateKey = 0;
            std::shared_ptr<databaseDelegate> mDelegate;


            std::map<NodeKey, node_type> mNodes;
            std::map<NodeKey, leaf_type> mLeafs;

        };
    }


}