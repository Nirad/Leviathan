/**
* This Source Code Form is part of UOP Package library by Nolok and subject to the terms of the
* GNU General Public License version 3. More info in the file "uoppackage.h", which is part of this source code package.
*/
#ifndef UOPBLOCK_H
#define UOPBLOCK_H

#include "uopfile.h"


namespace uopp
{

class UOPPackage;


class UOPBlock
{
    friend class UOPFile;
    friend class UOPPackage;

public:
    UOPBlock(UOPPackage* parent, unsigned int index = 0);
    ~UOPBlock();

    void read(std::ifstream& fin, UOPError* errorQueue = nullptr);
    bool readPackedData(std::ifstream& fin, UOPError* errorQueue = nullptr);
    void freePackedData();

    unsigned int searchByHash(unsigned long long hash) const;
    bool addFile(std::ifstream& fin, unsigned long long fileHash,       CompressionFlag compression, bool addDataHash, UOPError* errorQueue = nullptr);
    bool addFile(std::ifstream& fin, const std::string& packedFileName, CompressionFlag compression, bool addDataHash, UOPError* errorQueue = nullptr);

// Block structure
private:
    UOPPackage* m_parent;
    unsigned int m_index;
    unsigned int m_fileCount;
    std::vector<UOPFile*> m_files;
    unsigned long long m_nextBlockAddress;

    // Used only when creating a package
    unsigned int m_curFileIdx;

public:
    UOPPackage* getParent() const                   { return m_parent;              }
    unsigned int getIndex() const                   { return m_index;               }
    unsigned int getFilesCount() const              { return m_fileCount;           }
    UOPFile* getFile(unsigned int index) const      { return m_files[index];        }
    unsigned long long getNextBlockAddress() const  { return m_nextBlockAddress;    }
    bool isEmpty() const                            { return m_files.empty();       }

    // Iterators
    template <typename PointerType> class base_iterator;
    using iterator = base_iterator<UOPFile*>;
    iterator end();         // invalid iterator (obtained when incrementing an iterator to the last item)
    iterator begin();       // iterator to first item
    iterator back_it();     // iterator to last item
    using const_iterator = base_iterator<const UOPFile*>;
    const_iterator cend() const;
    const_iterator cbegin() const;
    const_iterator cback_it() const;
};


template <typename PointerType>
class UOPBlock::base_iterator
{
protected:
    const UOPBlock* m_block;
    unsigned int m_currentFileIdx;

public:
    base_iterator() = delete; // would construct an invalid iterator
    base_iterator(const UOPBlock* block, unsigned int currentFileIdx);
    ~base_iterator() = default;
    base_iterator(const base_iterator&) = default;                  // copy constructor
    base_iterator(base_iterator&&) noexcept = default;              // move constructor
    base_iterator& operator=(const base_iterator&) = default;       // copy assignment operator
    base_iterator& operator=(base_iterator&&) noexcept = default;   // move assignment operator
    bool operator==(const base_iterator&) const;
    bool operator!=(const base_iterator&) const;
    base_iterator operator++();      // pre-increment
    base_iterator operator++(int);   // post-increment
    base_iterator operator--();      // pre-decrement
    base_iterator operator--(int);   // post-decrement
    PointerType operator*();
};


// Iterator implementation

template <typename T>
UOPBlock::base_iterator<T>::base_iterator(const UOPBlock *block, unsigned int currentFileIdx) :
    m_block(block), m_currentFileIdx(currentFileIdx)
{
}

template <typename T>
bool UOPBlock::base_iterator<T>::operator==(const base_iterator& it) const
{
    return ((m_block == it.m_block) && (m_currentFileIdx == it.m_currentFileIdx));
}

template <typename T>
bool UOPBlock::base_iterator<T>::operator!=(const base_iterator& it) const
{
    return !(*this == it);
}

template <typename T>
UOPBlock::base_iterator<T> UOPBlock::base_iterator<T>::operator++()      // pre-increment
{
    if (m_currentFileIdx < m_block->getFilesCount() - 1)
    {
        ++m_currentFileIdx;
        return *this;
    }
    m_currentFileIdx = kInvalidIdx;
    return *this;
}

template <typename T>
UOPBlock::base_iterator<T> UOPBlock::base_iterator<T>::operator++(int)   // post-increment
{
    base_iterator oldIt = *this;
    ++(*this);  // do pre-increment
    return oldIt;
}

template <typename T>
UOPBlock::base_iterator<T> UOPBlock::base_iterator<T>::operator--()      // pre-decrement
{
    if (m_currentFileIdx > 0)
    {
        --m_currentFileIdx;
        return *this;
    }
    m_currentFileIdx = kInvalidIdx;
}

template <typename T>
UOPBlock::base_iterator<T> UOPBlock::base_iterator<T>::operator--(int)   // post-decrement
{
    base_iterator oldIt = *this;
    --(*this);  // do pre-increment
    return oldIt;
}

template <typename PointerType>
PointerType UOPBlock::base_iterator<PointerType>::operator*()
{
    return m_block->getFile(m_currentFileIdx);
}

}

#endif // UOPBLOCK_H
