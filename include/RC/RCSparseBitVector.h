/*
 * RCSparseBitVector.h
 *
 *  Created on: 16/07/2015
 *      Author: dye
 */

#ifndef RCSPARSEBITVECTOR_H_
#define RCSPARSEBITVECTOR_H_

#include <stack>
#include <cassert>
#include <cstring>
#include <algorithm>
#include <vector>
#include <list>
#include <unordered_map>
#include <iostream>


/*!
 * RCSparseBitVectorElement is employed by the RCSparseBitVector class.
 */
template<unsigned ElementSize = 128>
struct RCSparseBitVectorElement {
public:
    typedef unsigned long BitWord;
    enum {
        BITWORD_SIZE = sizeof(BitWord) * 8,
        BITWORDS_PER_ELEMENT = (ElementSize + BITWORD_SIZE - 1) / BITWORD_SIZE,
        BITS_PER_ELEMENT = ElementSize,
        BYTES_PER_ELEMENT = sizeof(BitWord) * BITWORDS_PER_ELEMENT
    };

    /// Constructors
    //@{
    RCSparseBitVectorElement() :
            Next(NULL), Prev(NULL), ElementIndex(0) {
        memset(&Bits[0], 0, BYTES_PER_ELEMENT);
    }
    explicit RCSparseBitVectorElement(unsigned Idx) :
            Next(NULL), Prev(NULL), ElementIndex(Idx) {
        memset(&Bits[0], 0, BYTES_PER_ELEMENT);
    }
    RCSparseBitVectorElement(const RCSparseBitVectorElement &RHS) :
        Next(NULL), Prev(NULL), ElementIndex(RHS.ElementIndex) {
        memcpy(&Bits[0], &RHS.Bits[0], BYTES_PER_ELEMENT);
    }
    //@}

    /// Destructor
    ~RCSparseBitVectorElement() {
    }

    /// Setter and getter of the next and previous nodes
    //@{
    inline RCSparseBitVectorElement<ElementSize> *getNext() const {
        return Next;
    }
    inline RCSparseBitVectorElement<ElementSize> *getPrev() const {
        return Prev;
    }
    inline void setNext(RCSparseBitVectorElement<ElementSize> *RHS) {
        Next = RHS;
    }
    inline void setPrev(RCSparseBitVectorElement<ElementSize> *RHS) {
        Prev = RHS;
    }
    //@}

    /// Comparison
    //@{
    inline bool operator==(const RCSparseBitVectorElement &RHS) const {
        if (ElementIndex != RHS.ElementIndex)
            return false;
        return !memcmp(&Bits[0], &RHS.Bits[0], BYTES_PER_ELEMENT);
    }
    inline bool operator!=(const RCSparseBitVectorElement &RHS) const {
        return !(*this == RHS);
    }
    //@}

    /// Get the bitmap content
    inline BitWord word(unsigned Idx) const {
        assert(Idx < BITWORDS_PER_ELEMENT);
        return Bits[Idx];
    }

    inline unsigned index() const {
        return ElementIndex;
    }

    inline bool empty() const {
        return !memcmp(&Bits[0], &zero[0], BYTES_PER_ELEMENT);
    }

    inline void set(unsigned Idx) {
        Bits[Idx / BITWORD_SIZE] |= 1L << (Idx % BITWORD_SIZE);
    }

    inline bool test_and_set(unsigned Idx) {
        bool old = test(Idx);
        if (!old) {
            set(Idx);
            return true;
        }
        return false;
    }

    inline void reset(unsigned Idx) {
        Bits[Idx / BITWORD_SIZE] &= ~(1L << (Idx % BITWORD_SIZE));
    }

    inline bool test(unsigned Idx) const {
        return Bits[Idx / BITWORD_SIZE] & (1L << (Idx % BITWORD_SIZE));
    }

    inline unsigned count() const {
        unsigned NumBits = 0;
        for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i)
            if (sizeof(BitWord) == 4)
                NumBits += CountPopulation_32(Bits[i]);
            else if (sizeof(BitWord) == 8)
                NumBits += CountPopulation_64(Bits[i]);
            else
                assert(0 && "Unsupported!");
        return NumBits;
    }

    /// find_first - Returns the index of the first set bit.
    inline int find_first() const {
        for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i)
            if (Bits[i] != 0) {
                if (sizeof(BitWord) == 4)
                    return i * BITWORD_SIZE + countTrailingZeros(Bits[i]);
                else if (sizeof(BitWord) == 8)
                    return i * BITWORD_SIZE + countTrailingZeros(Bits[i]);
                else
                    assert(0 && "Unsupported!");
            }
        assert(0 && "Illegal empty element");
    }

    /// find_next - Returns the index of the next set bit starting from the
    /// "Curr" bit. Returns -1 if the next set bit is not found.
    inline int find_next(unsigned Curr) const {
        if (Curr >= BITS_PER_ELEMENT)
            return -1;

        unsigned WordPos = Curr / BITWORD_SIZE;
        unsigned BitPos = Curr % BITWORD_SIZE;
        BitWord Copy = Bits[WordPos];
        assert(
                WordPos <= BITWORDS_PER_ELEMENT
                        && "Word Position outside of element");

        // Mask off previous bits.
        Copy &= ~0L << BitPos;

        if (Copy != 0) {
            if (sizeof(BitWord) == 4)
                return WordPos * BITWORD_SIZE + countTrailingZeros(Copy);
            else if (sizeof(BitWord) == 8)
                return WordPos * BITWORD_SIZE + countTrailingZeros(Copy);
            else
                assert(0 && "Unsupported!");
        }

        // Check subsequent words.
        for (unsigned i = WordPos + 1; i < BITWORDS_PER_ELEMENT; ++i)
            if (Bits[i] != 0) {
                if (sizeof(BitWord) == 4)
                    return i * BITWORD_SIZE + countTrailingZeros(Bits[i]);
                else if (sizeof(BitWord) == 8)
                    return i * BITWORD_SIZE + countTrailingZeros(Bits[i]);
                else
                    assert(0 && "Unsupported!");
            }
        return -1;
    }

    /// Union this element with RHS and return true if this one changed.
    inline bool unionWith(const RCSparseBitVectorElement &RHS) {
        bool changed = false;
        for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i) {
            BitWord old = changed ? 0 : Bits[i];

            Bits[i] |= RHS.Bits[i];
            if (!changed && old != Bits[i])
                changed = true;
        }
        return changed;
    }

    /// Check if we have any bits in common with RHS
    inline bool intersects(const RCSparseBitVectorElement &RHS) const {
        for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i) {
            if (RHS.Bits[i] & Bits[i])
                return true;
        }
        return false;
    }

    /// Intersect this Element with RHS and return true if this one changed.
    /// BecameZero is set to true if this element became all-zero bits.
    inline bool intersectWith(const RCSparseBitVectorElement &RHS, bool &BecameZero) {
        bool changed = false;
        bool allzero = true;

        BecameZero = false;
        for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i) {
            BitWord old = changed ? 0 : Bits[i];

            Bits[i] &= RHS.Bits[i];
            if (Bits[i] != 0)
                allzero = false;

            if (!changed && old != Bits[i])
                changed = true;
        }
        BecameZero = allzero;
        return changed;
    }

    /// Intersect this Element with the complement of RHS and return true if this
    /// one changed.  BecameZero is set to true if this element became all-zero
    /// bits.
    inline bool intersectWithComplement(const RCSparseBitVectorElement &RHS,
            bool &BecameZero) {
        bool changed = false;
        bool allzero = true;

        BecameZero = false;
        for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i) {
            BitWord old = changed ? 0 : Bits[i];

            Bits[i] &= ~RHS.Bits[i];
            if (Bits[i] != 0)
                allzero = false;

            if (!changed && old != Bits[i])
                changed = true;
        }
        BecameZero = allzero;
        return changed;
    }

    /// Three argument version of intersectWithComplement that intersects
    /// RHS1 & ~RHS2 into this element
    inline void intersectWithComplement(const RCSparseBitVectorElement &RHS1,
            const RCSparseBitVectorElement &RHS2, bool &BecameZero) {
        bool allzero = true;

        BecameZero = false;
        for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i) {
            Bits[i] = RHS1.Bits[i] & ~RHS2.Bits[i];
            if (Bits[i] != 0)
                allzero = false;
        }
        BecameZero = allzero;
    }

    /// Get a hash value for this element;
    inline uint64_t getHashValue() const {
        uint64_t HashVal = 0;
        for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i) {
            HashVal ^= Bits[i];
        }
        return HashVal;
    }

    /// Return a pointer to the Bits array
    inline BitWord* getBits() {
        return &Bits[0];
    }

    /// Increment the ElementIndex by the given amount
    inline void incIndex(unsigned x) {
        ElementIndex += x;
    }

    /// Initialize the element
    //@{
    inline void init(unsigned idx) {
        ElementIndex = idx;
        memset(&Bits[0], 0, BYTES_PER_ELEMENT);
    }
    inline void init(const RCSparseBitVectorElement &RHS) {
        ElementIndex = RHS.ElementIndex;
        memcpy(&Bits[0], &RHS.Bits[0], BYTES_PER_ELEMENT);
    }
    //@}

private:

    /// Count the number of trailing zeros for val.
    static int countTrailingZeros(BitWord val) {
        if (0 == val)   return sizeof(BitWord);
        static_assert((8 == sizeof(BitWord)), "BitWord must have 64-bit width.");
        return __builtin_ctzl(val);
    }

    /// Next and previous node links
    //@{
    RCSparseBitVectorElement<ElementSize> *Next;
    RCSparseBitVectorElement<ElementSize> *Prev;
    //@}

    /// Index of Element in terms of where first bit starts.
    unsigned ElementIndex;

    /// bitmap content
    BitWord Bits[BITWORDS_PER_ELEMENT];

    /// to compare an Element against zero
    static BitWord zero[BITWORDS_PER_ELEMENT];
};


template<unsigned ElementSize> RCSparseBitVectorElement<>::BitWord
RCSparseBitVectorElement<ElementSize>::zero[] = { 0 };


/*!
 * RCSparseBitVector is a bitmap implementation which is similar to the
 * LLVM's built-in SparseBitVector, with the advantages to easily compute
 * the hash value of a bitmap.
 */
template<unsigned ElementSize = 128>
class RCSparseBitVector {
    typedef std::list<RCSparseBitVectorElement<ElementSize> > ElementList;
    typedef typename ElementList::iterator ElementListIter;
    typedef typename ElementList::const_iterator ElementListConstIter;
    typedef typename RCSparseBitVectorElement<ElementSize>::BitWord BitWord;
    enum {
        BITWORD_SIZE = RCSparseBitVectorElement<ElementSize>::BITWORD_SIZE,
        WORDSZ = BITWORD_SIZE,
        NWORDS = RCSparseBitVectorElement<ElementSize>::BITWORDS_PER_ELEMENT,
        BYTES_PER_ELEMENT = RCSparseBitVectorElement<ElementSize>::BYTES_PER_ELEMENT
    };

    // Pointer to our current Element.
    ElementListIter CurrElementIter;
    ElementList Elements;

    std::stack<RCSparseBitVectorElement<ElementSize>*> alloc_stack;

    RCSparseBitVectorElement<ElementSize>* getNewElement(unsigned idx) {
        RCSparseBitVectorElement<ElementSize> *ret = 0;

        if (alloc_stack.empty()) {
            ret = new RCSparseBitVectorElement<ElementSize>(idx);
        } else {
            ret = alloc_stack.top();
            alloc_stack.pop();
            assert(ret);
            ret->init(idx);
        }

        return ret;
    }

    RCSparseBitVectorElement<ElementSize>* getNewElement(
            const RCSparseBitVectorElement<ElementSize>& cpy) {
        RCSparseBitVectorElement<ElementSize> *ret = 0;

        if (alloc_stack.empty()) {
            ret = new RCSparseBitVectorElement<ElementSize>(cpy);
        } else {
            ret = alloc_stack.top();
            alloc_stack.pop();
            assert(ret);
            ret->init(cpy);
        }

        return ret;
    }

    void eraseElement(RCSparseBitVectorElement<ElementSize> *el) {
        assert(el);
        alloc_stack.push(el);
    }

    void eraseElement(const ElementListIter &it) {
        RCSparseBitVectorElement<ElementSize> *el = &*Elements.erase(it);
        assert(el);
        alloc_stack.push(el);
    }

    void eraseElement(const ElementListIter &bgn, const ElementListIter &end) {
        ElementListIter i = bgn, k;
        while (i != end) {
            k = i;
            ++i;
            eraseElement(k);
        }
    }

    // This is like std::lower_bound, except we do linear searching from the
    // current position.
    ElementListIter FindLowerBound(unsigned ElementIndex) {

        if (Elements.empty()) {
            CurrElementIter = Elements.begin();
            return Elements.begin();
        }

        // Make sure our current iterator is valid.
        if (CurrElementIter == Elements.end())
            --CurrElementIter;

        // Search from our current iterator, either backwards or forwards,
        // depending on what element we are looking for.
        ElementListIter ElementIter = CurrElementIter;
        if (CurrElementIter->index() == ElementIndex) {
            return ElementIter;
        } else if (CurrElementIter->index() > ElementIndex) {
            while (ElementIter != Elements.begin()
                    && ElementIter->index() > ElementIndex)
                --ElementIter;
        } else {
            while (ElementIter != Elements.end()
                    && ElementIter->index() < ElementIndex)
                ++ElementIter;
        }
        CurrElementIter = ElementIter;
        return ElementIter;
    }

    // Iterator to walk set bits in the bitmap.  This iterator is a lot uglier
    // than it would be, in order to be efficient.
    class RCSparseBitVectorIterator {
    private:
        bool AtEnd;

        const RCSparseBitVector<ElementSize> *BitVector;

        // Current element inside of bitmap.
        ElementListConstIter Iter;

        // Current bit number inside of our bitmap.
        unsigned BitNumber;

        // Current word number inside of our element.
        unsigned WordNumber;

        // Current bits from the element.
        typename RCSparseBitVectorElement<ElementSize>::BitWord Bits;

        // Move our iterator to the first non-zero bit in the bitmap.
        void AdvanceToFirstNonZero() {
            if (AtEnd)
                return;
            if (BitVector->Elements.empty()) {
                AtEnd = true;
                return;
            }
            Iter = BitVector->Elements.begin();
            BitNumber = Iter->index() * ElementSize;
            unsigned BitPos = Iter->find_first();
            BitNumber += BitPos;
            WordNumber = (BitNumber % ElementSize) / BITWORD_SIZE;
            Bits = Iter->word(WordNumber);
            Bits >>= BitPos % BITWORD_SIZE;
        }

        // Move our iterator to the next non-zero bit.
        void AdvanceToNextNonZero() {
            if (AtEnd)
                return;

            while (Bits && !(Bits & 1)) {
                Bits >>= 1;
                BitNumber += 1;
            }

            // See if we ran out of Bits in this word.
            if (!Bits) {
                int NextSetBitNumber = Iter->find_next(BitNumber % ElementSize);
                // If we ran out of set bits in this element, move to next element.
                if (NextSetBitNumber == -1 || (BitNumber % ElementSize == 0)) {
                    ++Iter;
                    WordNumber = 0;

                    // We may run out of elements in the bitmap.
                    if (Iter == BitVector->Elements.end()) {
                        AtEnd = true;
                        return;
                    }
                    // Set up for next non zero word in bitmap.
                    BitNumber = Iter->index() * ElementSize;
                    NextSetBitNumber = Iter->find_first();
                    BitNumber += NextSetBitNumber;
                    WordNumber = (BitNumber % ElementSize) / BITWORD_SIZE;
                    Bits = Iter->word(WordNumber);
                    Bits >>= NextSetBitNumber % BITWORD_SIZE;
                } else {
                    WordNumber = (NextSetBitNumber % ElementSize)
                            / BITWORD_SIZE;
                    Bits = Iter->word(WordNumber);
                    Bits >>= NextSetBitNumber % BITWORD_SIZE;
                    BitNumber = Iter->index() * ElementSize;
                    BitNumber += NextSetBitNumber;
                }
            }
        }
    public:
        // Preincrement.
        inline RCSparseBitVectorIterator& operator++() {
            ++BitNumber;
            Bits >>= 1;
            AdvanceToNextNonZero();
            return *this;
        }

        // Postincrement.
        inline RCSparseBitVectorIterator operator++(int) {
            RCSparseBitVectorIterator tmp = *this;
            ++*this;
            return tmp;
        }

        // Return the current set bit number.
        unsigned operator*() const {
            return BitNumber;
        }

        bool operator==(const RCSparseBitVectorIterator &RHS) const {
            // If they are both at the end, ignore the rest of the fields.
            if (AtEnd && RHS.AtEnd)
                return true;
            // Otherwise they are the same if they have the same bit number and
            // bitmap.
            return AtEnd == RHS.AtEnd && RHS.BitNumber == BitNumber;
        }

        bool operator!=(const RCSparseBitVectorIterator &RHS) const {
            return !(*this == RHS);
        }

        RCSparseBitVectorIterator(const RCSparseBitVector<ElementSize> *RHS,
                bool end = false) :
                BitVector(RHS) {
            Iter = BitVector->Elements.begin();
            BitNumber = 0;
            Bits = 0;
            WordNumber = ~0;
            AtEnd = end;
            AdvanceToFirstNonZero();
        }
    };
public:
    typedef RCSparseBitVectorIterator iterator;

    RCSparseBitVector() {
        CurrElementIter = Elements.begin();
    }

    ~RCSparseBitVector() {
        ElementListIter i = Elements.begin(), e = Elements.end(), k;
        while (i != e) {
            k = i;
            ++i;
            Elements.erase(k);
        }
    }

    // RCSparseBitVector copy ctor.
    RCSparseBitVector(const RCSparseBitVector &RHS) {
        ElementListConstIter ElementIter = RHS.Elements.begin();
        while (ElementIter != RHS.Elements.end()) {
            RCSparseBitVectorElement<ElementSize> *el = getNewElement(
                    *ElementIter);
            Elements.push_back(*el);
            ++ElementIter;
        }

        CurrElementIter = Elements.begin();

        assert(alloc_stack.empty());
    }

    // Test, Reset, and Set a bit in the bitmap.
    bool test(unsigned Idx) {
        if (Elements.empty())
            return false;

        unsigned ElementIndex = Idx / ElementSize;
        ElementListIter ElementIter = FindLowerBound(ElementIndex);

        // If we can't find an element that is supposed to contain this bit, there
        // is nothing more to do.
        if (ElementIter == Elements.end()
                || ElementIter->index() != ElementIndex)
            return false;
        return ElementIter->test(Idx % ElementSize);
    }

    void reset(unsigned Idx) {
        if (Elements.empty())
            return;

        unsigned ElementIndex = Idx / ElementSize;
        ElementListIter ElementIter = FindLowerBound(ElementIndex);

        // If we can't find an element that is supposed to contain this bit, there
        // is nothing more to do.
        if (ElementIter == Elements.end()
                || ElementIter->index() != ElementIndex)
            return;
        ElementIter->reset(Idx % ElementSize);

        // When the element is zeroed out, delete it.
        if (ElementIter->empty()) {
            ++CurrElementIter;
            eraseElement(ElementIter);
        }
    }

    void set(unsigned Idx) {
        unsigned ElementIndex = Idx / ElementSize;
        RCSparseBitVectorElement<ElementSize> *Element;
        ElementListIter ElementIter;
        if (Elements.empty()) {
            Element = getNewElement(ElementIndex);
            ElementIter = Elements.insert(Elements.end(), *Element);

        } else {
            ElementIter = FindLowerBound(ElementIndex);

            if (ElementIter == Elements.end()
                    || ElementIter->index() != ElementIndex) {
                Element = getNewElement(ElementIndex);
                // We may have hit the beginning of our RCSparseBitVector, in
                // which case, we may need to insert right after this element,
                // which requires moving the current iterator forward one,
                // because insert does insert before.
                if (ElementIter != Elements.end()
                        && ElementIter->index() < ElementIndex)
                    ElementIter = Elements.insert(++ElementIter, *Element);
                else
                    ElementIter = Elements.insert(ElementIter, *Element);
            }
        }
        CurrElementIter = ElementIter;

        ElementIter->set(Idx % ElementSize);
    }

    bool test_and_set(unsigned Idx) {
        bool old = test(Idx);
        if (!old) {
            set(Idx);
            return true;
        }
        return false;
    }

    bool operator!=(const RCSparseBitVector &RHS) const {
        return !(*this == RHS);
    }

    bool operator==(const RCSparseBitVector &RHS) const {
        ElementListConstIter Iter1 = Elements.begin();
        ElementListConstIter Iter2 = RHS.Elements.begin();

        for (; Iter1 != Elements.end() && Iter2 != RHS.Elements.end();
                ++Iter1, ++Iter2) {
            if (*Iter1 != *Iter2)
                return false;
        }
        return Iter1 == Elements.end() && Iter2 == RHS.Elements.end();
    }

    RCSparseBitVector& operator=(const RCSparseBitVector &RHS) {
        clear();

        for (ElementListConstIter it = RHS.Elements.begin(), end =
                RHS.Elements.end(); it != end; ++it) {
            Elements.insert(Elements.end(), getNewElement(*it));
        }

        return *this;
    }

    /// Union our bitmap with the RHS and return true if we changed.
    bool operator|=(const RCSparseBitVector &RHS) {
        bool changed = false;
        ElementListIter Iter1 = Elements.begin();
        ElementListConstIter Iter2 = RHS.Elements.begin();

        // If RHS is empty, we are done
        if (RHS.Elements.empty())
            return false;

        while (Iter2 != RHS.Elements.end()) {
            if (Iter1 == Elements.end() || Iter1->index() > Iter2->index()) {
                Elements.insert(Iter1, *getNewElement(*Iter2));
                ++Iter2;
                changed = true;
            } else if (Iter1->index() == Iter2->index()) {
                changed |= Iter1->unionWith(*Iter2);
                ++Iter1;
                ++Iter2;
            } else {
                ++Iter1;
            }
        }
        CurrElementIter = Elements.begin();
        return changed;
    }

    /// Intersect our bitmap with the RHS and return true if ours changed.
    bool operator&=(const RCSparseBitVector &RHS) {
        if (this == &RHS)   return false;

        bool changed = false;
        ElementListIter Iter1 = Elements.begin();
        ElementListConstIter Iter2 = RHS.Elements.begin();

        // Check if both bitmaps are empty.
        if (Elements.empty() && RHS.Elements.empty())
            return false;

        // Loop through, intersecting as we go, erasing elements when necessary.
        while (Iter2 != RHS.Elements.end()) {
            if (Iter1 == Elements.end()) {
                CurrElementIter = Elements.begin();
                return changed;
            }

            if (Iter1->index() > Iter2->index()) {
                ++Iter2;
            } else if (Iter1->index() == Iter2->index()) {
                bool BecameZero;
                changed |= Iter1->intersectWith(*Iter2, BecameZero);
                if (BecameZero) {
                    ElementListIter IterTmp = Iter1;
                    ++Iter1;
                    eraseElement(IterTmp);
                } else {
                    ++Iter1;
                }
                ++Iter2;
            } else {
                ElementListIter IterTmp = Iter1;
                ++Iter1;
                eraseElement(IterTmp);
            }
        }
        eraseElement(Iter1, Elements.end());
        CurrElementIter = Elements.begin();
        return changed;
    }

    /// Intersect our bitmap with the complement of the RHS and return
    /// true if ours changed.
    bool intersectWithComplement(const RCSparseBitVector &RHS) {
        bool changed = false;
        ElementListIter Iter1 = Elements.begin();
        ElementListConstIter Iter2 = RHS.Elements.begin();

        // If either our bitmap or RHS is empty, we are done
        if (Elements.empty() || RHS.Elements.empty())
            return false;

        // Loop through, intersecting as we go, erasing elements when necessary.
        while (Iter2 != RHS.Elements.end()) {
            if (Iter1 == Elements.end()) {
                CurrElementIter = Elements.begin();
                return changed;
            }

            if (Iter1->index() > Iter2->index()) {
                ++Iter2;
            } else if (Iter1->index() == Iter2->index()) {
                bool BecameZero;
                changed |= Iter1->intersectWithComplement(*Iter2, BecameZero);
                if (BecameZero) {
                    ElementListIter IterTmp = Iter1;
                    ++Iter1;
                    eraseElement(IterTmp);
                } else {
                    ++Iter1;
                }
                ++Iter2;
            } else {
                ++Iter1;
            }
        }
        CurrElementIter = Elements.begin();
        return changed;
    }

    bool intersectWithComplement(
            const RCSparseBitVector<ElementSize> *RHS) const {
        return intersectWithComplement(*RHS);
    }

    /// Three argument version of intersectWithComplement.  Result of
    /// RHS1 & ~RHS2 is stored into this bitmap.
    void intersectWithComplement(const RCSparseBitVector<ElementSize> &RHS1,
            const RCSparseBitVector<ElementSize> &RHS2) {
        clear();
        ElementListConstIter Iter1 = RHS1.Elements.begin();
        ElementListConstIter Iter2 = RHS2.Elements.begin();

        // If RHS1 is empty, we are done
        // If RHS2 is empty, we still have to copy RHS1
        if (RHS1.Elements.empty())
            return;

        // Loop through, intersecting as we go, erasing elements when necessary.
        while (Iter2 != RHS2.Elements.end()) {
            if (Iter1 == RHS1.Elements.end())
                return;

            if (Iter1->index() > Iter2->index()) {
                ++Iter2;
            } else if (Iter1->index() == Iter2->index()) {
                bool BecameZero = false;
                RCSparseBitVectorElement<ElementSize> *NewElement =
                        getNewElement(Iter1->index());
                NewElement->intersectWithComplement(*Iter1, *Iter2, BecameZero);
                if (!BecameZero) {
                    Elements.push_back(NewElement);
                } else
                    eraseElement(NewElement);
                ++Iter1;
                ++Iter2;
            } else {
                RCSparseBitVectorElement<ElementSize> *NewElement =
                        getNewElement(*Iter1);
                Elements.push_back(NewElement);
                ++Iter1;
            }
        }

        // copy the remaining elements
        while (Iter1 != RHS1.Elements.end()) {
            RCSparseBitVectorElement<ElementSize> *NewElement = getNewElement(
                    *Iter1);
            Elements.push_back(NewElement);
            ++Iter1;
        }

        return;
    }

    /// Return true if we share any bits in common with RHS
    //@{
    void intersectWithComplement(const RCSparseBitVector<ElementSize> *RHS1,
            const RCSparseBitVector<ElementSize> *RHS2) {
        intersectWithComplement(*RHS1, *RHS2);
    }
    bool intersects(const RCSparseBitVector<ElementSize> *RHS) const {
        return intersects(*RHS);
    }
    bool intersects(const RCSparseBitVector<ElementSize> &RHS) const {
        ElementListConstIter Iter1 = Elements.begin();
        ElementListConstIter Iter2 = RHS.Elements.begin();

        // Check if both bitmaps are empty.
        if (Elements.empty() && RHS.Elements.empty())
            return false;

        // Loop through, intersecting stopping when we hit bits in common.
        while (Iter2 != RHS.Elements.end()) {
            if (Iter1 == Elements.end())
                return false;

            if (Iter1->index() > Iter2->index()) {
                ++Iter2;
            } else if (Iter1->index() == Iter2->index()) {
                if (Iter1->intersects(*Iter2))
                    return true;
                ++Iter1;
                ++Iter2;
            } else {
                ++Iter1;
            }
        }
        return false;
    }
    //@}

    /// Return the first set bit in the bitmap.  Return -1 if no bits are set.
    int find_first() const {
        if (Elements.empty())
            return -1;
        const RCSparseBitVectorElement<ElementSize> &First = *(Elements.begin());
        return (First.index() * ElementSize) + First.find_first();
    }

    /// Return true if the RCSparseBitVector is empty
    bool empty() const {
        return Elements.empty();
    }

    unsigned count() const {
        unsigned BitCount = 0;
        for (ElementListConstIter Iter = Elements.begin();
                Iter != Elements.end(); ++Iter)
            BitCount += Iter->count();

        return BitCount;
    }

    /// Iterators
    //@{
    iterator begin() const {
        return iterator(this);
    }
    iterator end() const {
        return iterator(this, true);
    }
    //@}

    /// Compute a hash value for this bitmap.
    uint64_t getHashValue() const {
        uint64_t HashVal = 0;
        for (ElementListConstIter Iter = Elements.begin();
                Iter != Elements.end(); ++Iter) {
            HashVal ^= Iter->index();
            HashVal ^= Iter->getHashValue();
        }
        return HashVal;
    }

    bool operator>>=(unsigned off) {
        if (Elements.empty() || off == 0) {
            return false;
        }

        int els = off / ElementSize; // whole BitArrays contained in offset

        if (els) {
            for (ElementListIter i = Elements.begin(), e = Elements.end();
                    i != e; ++i) { // first shift by whole BitArrays
                i->incIndex(els);
            }

            off %= ElementSize; // now we'll shift by the remainder
            if (off == 0) {
                return true;
            }
        }

        int whole = off / WORDSZ;    // whole words contained in offset
        int msb_sz = off % WORDSZ;    // most-significant bits
        int lsb_sz = WORDSZ - msb_sz; // least-significant bits

        // NOTE: if lsb_sz == WORDSZ then lsb and msb will be incorrect,
        // because shifting a word by >= the word size is undefined (and
        // gcc just leaves the word alone without shifting at all); this
        // doesn't matter because we don't need lsb and msb if lsb_sz ==
        // WORDSZ, see the code below for details

        BitWord lsb = (1 << lsb_sz) - 1;  // lsb mask
        BitWord msb = ~lsb;               // msb mask

        int i, j;

        BitWord buffer[2 * NWORDS];
        BitWord *buff = &buffer[0];

        memset(buff, 0, BYTES_PER_ELEMENT * 2);

        ElementListIter ii = Elements.begin(), k = Elements.end();

        while (ii != Elements.end()) {
            BitWord *bits = ii->getBits();

            // copy bits to buff at offset; if we're shifting by whole words
            // take advantage to make the copy more efficient
            //
            if (off % WORDSZ == 0) {
                memcpy(&buff[whole], bits, BYTES_PER_ELEMENT);
            } else {
                BitWord prev = 0; // msb bits from previous word, shifted to lsb

                for (i = NWORDS + whole, j = NWORDS - 1; i > whole; --i, --j) {
                    buff[i] = ((bits[j] & msb) >> lsb_sz) | prev;
                    prev = (bits[j] & lsb) << msb_sz;
                }

                buff[i] |= prev;
            }

            //
            // if buff_1 is all 0 erase this element
            // else memcpy buff_1 to this element
            //

            bool not_zero = memcmp(&buff[0],
                    &RCSparseBitVectorElement<ElementSize>::zero[0],
                    BYTES_PER_ELEMENT);

            if (not_zero) {
                memcpy(bits, buff, BYTES_PER_ELEMENT);
            } else {
                k = ii;
            }

            // if buff_2 is not all 0
            //   if next element is in series
            //     memcpy buff_2 to buff_1, memset buff_2 to 0
            //   else insert new next element, init to buff_2, memset buff to 0
            // else memset buff_1 to 0

            not_zero = memcmp(&buff[NWORDS],
                    &RCSparseBitVectorElement<ElementSize>::zero[0],
                    BYTES_PER_ELEMENT);

            if (not_zero) { // carry-over to next Element
                if (ii->getNext()->index() == ii->index() + 1) { // adjacent Element
                    memcpy(buff, &buff[NWORDS], BYTES_PER_ELEMENT);
                    memset(&buff[NWORDS], 0, BYTES_PER_ELEMENT);
                } else { // no adjacent Element
                    int idx = ii->index();
                    ii = Elements.insert(++ii, getNewElement(idx + 1));
                    bits = ii->getBits();
                    memcpy(bits, &buff[NWORDS], BYTES_PER_ELEMENT);
                    memset(buff, 0, BYTES_PER_ELEMENT * 2);
                }
            } else {
                memset(buff, 0, BYTES_PER_ELEMENT);
            }

            // erase this element if necessary
            //
            if (k != Elements.end()) {
                ++ii;
                eraseElement(k);
                k = Elements.end();
            } else {
                ++ii;
            }
        }

        CurrElementIter = Elements.begin();

        return true;
    }

    void clear() {
        eraseElement(Elements.begin(), Elements.end());
        CurrElementIter = Elements.begin();
    }

    /// return true iff there is exactly one bit set
    bool single_bit_p() {
        ElementListIter i = Elements.begin();

        if (i != Elements.end() && i->getNext()->index() == ~0U) {
            return (i->count() == 1);
        }

        return false;
    }

    /// clear bits [bgn..bgn+cnt), taken largely from GCC's bitmap.c
    void clear_range(unsigned bgn, unsigned cnt) {
        if (cnt == 0 || Elements.empty()) {
            return;
        }

        unsigned end = bgn + cnt;
        unsigned bgn_idx = bgn / ElementSize, end_idx = (end - 1) / ElementSize;
        ElementListIter it = FindLowerBound(bgn_idx), k;

        if (it == Elements.end()) {
            return;
        }
        if (it->index() < bgn_idx) {
            ++it;
        }

        while (it != Elements.end() && it->index() <= end_idx) {
            unsigned el_bgn = it->index() * ElementSize;
            unsigned el_end = el_bgn + ElementSize;

            if (el_bgn >= bgn && el_end <= end) {
                k = it;
                ++it;
                eraseElement(k);
            } else {
                unsigned bgn_word, end_word;
                BitWord bgn_msk, end_msk, *bits = it->getBits();

                if (el_bgn <= bgn) {
                    bgn_word = (bgn - el_bgn) / WORDSZ;
                    bgn_msk = ~((1 << (bgn % WORDSZ)) - 1);
                } else {
                    bgn_word = 0;
                    bgn_msk = ~0U;
                }

                if (el_end <= end) {
                    end_word = NWORDS - 1;
                    end_msk = ~0U;
                } else {
                    end_word = (end - el_bgn) / WORDSZ;
                    end_msk = (1 << (end % WORDSZ)) - 1;
                }

                if (bgn_word == end_word) {
                    bits[bgn_word] &= ~(bgn_msk & end_msk);
                } else {
                    bits[bgn_word] &= ~bgn_msk;
                    bits[end_word] &= ~end_msk;
                    for (unsigned i = bgn_word + 1; i < end_word; ++i) {
                        bits[i] = 0;
                    }
                }

                if (it->empty()) {
                    k = it;
                    ++it;
                    eraseElement(k);
                } else {
                    ++it;
                }
            }
        }

        CurrElementIter = it;
    }

    /// set the bits [bgn..bgn+cnt), taken largely from GCC's bitmap.c
    void set_range(unsigned bgn, unsigned cnt) {
        if (cnt == 0) {
            return;
        }

        unsigned end = bgn + cnt;
        unsigned bgn_idx = bgn / ElementSize, end_idx = (end - 1) / ElementSize;
        ElementListIter it = FindLowerBound(bgn_idx);

        if (it != Elements.end() && it->index() < bgn_idx) {
            ++it;
        }

        for (unsigned i = bgn_idx; i <= end_idx; ++i, ++it) {
            if (it == Elements.end() || it->index() != i) {
                it = Elements.insert(it, getNewElement(i));
            }

            unsigned el_bgn = i * ElementSize;
            unsigned el_end = el_bgn + ElementSize;

            unsigned bgn_word, end_word;
            BitWord bgn_msk, end_msk, *bits = it->getBits();

            if (el_bgn <= bgn) {
                bgn_word = (bgn - el_bgn) / WORDSZ;
                bgn_msk = ~((1 << (bgn % WORDSZ)) - 1);
            } else {
                bgn_word = 0;
                bgn_msk = ~0U;
            }

            if (el_end <= end) {
                end_word = NWORDS - 1;
                end_msk = ~0U;
            } else {
                end_word = (end - el_bgn) / WORDSZ;
                end_msk = (1 << (end % WORDSZ)) - 1;
            }

            if (bgn_word == end_word) {
                bits[bgn_word] |= (bgn_msk & end_msk);
            } else {
                bits[bgn_word] |= bgn_msk;
                bits[end_word] |= end_msk;
                for (unsigned j = bgn_word + 1; j < end_word; ++j) {
                    bits[j] = ~0U;
                }
            }
        }

        CurrElementIter = it;
    }

    unsigned num_els() {
        return Elements.size();
    }

    unsigned stack_size() {
        return alloc_stack.size();
    }

    /// Dump RCSparseBitVector to a stream
    void print(std::ostream &out) const {
        out << "[ ";
        typename RCSparseBitVector<ElementSize>::iterator bi;
        for (bi = begin(); bi != end(); ++bi) {
            out << *bi << " ";
        }
        out << " ]\n";
    }
};


namespace std { // begin std namespace

/// Define a hash function for RCSparseBitVector
template<>
struct hash<RCSparseBitVector<> > {
    size_t operator()(const RCSparseBitVector<> &s) const {
        return (size_t) s.getHashValue();
    }
};

} // end std namespace

#endif /* RCSPARSEBITVECTOR_H_ */
