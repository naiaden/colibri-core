#ifndef COLIBRIPATTERNSTORE_H
#define COLIBRIPATTERNSTORE_H

#include <string>
#include <iostream>
#include <ostream>
#include <istream>
#include <unordered_map>
#include <vector>
#include <set>
#include <map>
#include <array>
#include <unordered_set>
#include <iomanip> // contains setprecision()
#include <exception>
#include <algorithm>
#include "common.h"
#include "pattern.h"
#include "datatypes.h"
#include "classdecoder.h"
#include "classencoder.h"

/***********************************************************************************/

/**
 * @file patternstore.h
 * \brief Contains lower-level containers for patterns.
 *
 * @author Maarten van Gompel (proycon) <proycon@anaproy.nl>
 * 
 * @section LICENSE
 * Licensed under GPLv3
 *
 * @section DESCRIPTION
 * Contains lower-level containers for patterns
 */

enum StoreType { //TODO: check if used?
    PATTERNBASED = 0,
    POINTERBASED = 1,
};

/**
 * \brief This class corresponds to a single token in a corpus, it holds the token class and the position.
 */
class IndexPattern { 
    public:
        IndexReference ref; ///< The position of this token in the corpus
        unsigned char * data; ///< pointer to the data
        //uint32_t cls; ///< The content of this token (a class from a particular class encoding)
    
        /**
         * Constructor from a unigram Pattern 
         * @param ref The position in the corpus
         * @param pattern A unigram
         */
        IndexPattern(const IndexReference & ref, const PatternPointer & pattern) {
            this->ref = ref;
            this->data = pattern.data;
        }
        IndexPattern(const IndexReference & ref,  unsigned char * data) {
            this->ref = ref;
            this->data = data;
        }
        /**
         * Constructor from an integer 
         * @param ref The position in the corpus
         * @param cls A class from a particular class encoding
         */
        IndexPattern(const IndexReference & ref, uint32_t cls) {
            this->ref = ref;
            this->cls = cls;
        }
        IndexPattern(const IndexReference & ref) {
            this->ref = ref;
            this->cls = 0;
        }

        /**
         * Obtain a unigram Pattern
         */
        Pattern pattern() {
            unsigned char * buffer = new unsigned char[16]; //small buffer, but cls can't be too big anyhow
            const unsigned char classlength = inttobytes(buffer, (unsigned int) cls);
            Pattern p = Pattern(buffer, classlength);
            delete[] buffer;
            return p;
        }

        /**
         * Tests if two IndexPatterns describe the same position, in which case
         * they are considered equal regardless of the class content!
         * Unsuitable for use in containers where one reference is ambiguous,
         * designed for IndexedCorpus
         */
        bool operator==(const IndexPattern &other) const { return (this->ref == other.ref); };
        bool operator==(const IndexReference &other) const { return (this->ref == other); };
        bool operator!=(const IndexPattern &other) const { return (this->ref != other.ref); };
        bool operator!=(const IndexReference &other) const { return (this->ref != other); };

        bool operator< (const IndexPattern& other) const {
            return (this->ref < other.ref);
        }
        bool operator< (const IndexReference& other) const {
            return (this->ref < other);
        }
        bool operator> (const IndexPattern& other) const {
            return (this->ref > other.ref);
        }
        bool operator> (const IndexReference& other) const {
            return (this->ref > other);
        }
};

/**
 * \brief Class for reading an entire (class encoded) corpus into memory. 
 * It provides a reverse index by IndexReference. The reverse index stores positions and unigrams.
 */
class IndexedCorpus {
    protected:
        unsigned char * corpus;
        unsigned int corpussize; //in bytes
		PatternPointer * patternpointer;
        unsigned int totaltokens;
        std::map<uint32_t,unsigned char*> sentenceindex; //sentence pointers
    public:
        IndexedCorpus() {
            corpus = NULL
            corpussize = 0;
		    totaltokens = 0;
			patternpointer = NULL;
        };

        /*
         * Read an indexed corpus from stream. The stream must correspond to an
         * encoded corpus (*.colibri.dat)
         */
        IndexedCorpus(std::istream *in, bool debug = false);
        /*
         * Read an indexed corpus from file. The filename must correspond to an
         * encoded corpus (*.colibri.dat)
         */
        IndexedCorpus(std::string filename, bool debug = false);

        ~IndexedCorpus() { 
			if (corpus != NULL) delete[] corpus; 
			if (patternpointer != NULL) delete patternpointer; 
		}
        
        /*
         * Read an indexed corpus from stream. The stream must correspond to an
         * encoded corpus (*.colibri.dat)
         */
        void load(std::istream *in, bool debug = false);

        /*
         * Read an indexed corpus from file. The filename must correspond to an
         * encoded corpus (*.colibri.dat)
         */
        void load(std::string filename, bool debug = false);

        /** 
         * Low-level function, returns a data pointer given an IndexReference.
         * Returns NULL when the index does not exist.
         * Use getpattern() instead.
         */
        unsigned char * getpointer(const IndexReference & begin) const;

        /**
         * Returns a pattern starting at the provided position and of the
         * specified length.
         */
        PatternPointer getpattern(const IndexReference & begin, int length=1) const;

        /**
         * Returns a pattern starting at the provided position and of the
         * specified length.
         */
        Pattern getpattern(const IndexReference & begin, int length=1) const;

        /**
         * Get the sentence (or whatever other unit your data employs)
         * specified by the given index. Sentences start at 1.
         */
        Pattern getsentence(int sentence) const; //returns sentence as a pattern
        PatternPointer getsentence(int sentence) const; //returns sentence as a pattern pointer
         
        /**
         * Returns all positions at which the pattern occurs. Up to a certain
         * number of maximum matches if desired. Note that this iterates over
         * the entire corpus and is by far not as efficient as a proper pattern
         * model.
         * @param sentence Restrict to a particular sentence (0=all sentences, default)
         */
        std::vector<IndexReference> findpattern(const Pattern & pattern, int maxmatches=0, uint32_t sentence = 0); 

        void findpattern(const Pattern & pattern, uint32_t sentence, unsigned char * sentencedata, int maxmatches=0); 
        /**
         * Returns the length of the sentence (or whatever other unit your data
         * employs) at the given sentence index (starts at 1)
         */
        int sentencelength(int sentence) const;  
        int sentencelength(unsigned char * sentencebegin) const;  

        /**
         * Return the total number of sentences (or whatever other unit
         * delimites your data) in the corpus.
         */
        unsigned int sentences() const { return sentenceindex.size(); }  //returns the number of sentences (1-indexed)


		/**
		* Iterators
		*/
        class iterator {
            public:
                typedef iterator self_type;
                typedef PatternPointer value_type;
                typedef PatternPointer& reference;
                typedef PatternPointer* pointer;
                typedef std::forward_iterator_tag iterator_category;
                typedef int difference_type;
                iterator(pointer ptr) : ptr_(ptr) { }
                iterator(reference ref) : ptr_(ref) { }
				self_type operator++() { (*ptr_)++; return *this; }
                self_type operator++(int junk) { self_type tmpiter = *this; (*ptr_)++; return *tmpiter; }
                reference operator*() { return *ptr_; }
                pointer operator->() { return ptr_; }
                bool operator==(const self_type& rhs) { return *ptr_ == *rhs; }
                bool operator!=(const self_type& rhs) { return *ptr_ != *rhs; }
            private:
                pointer ptr_;
        };
    
        /*
         * Returns the begin iterator over the corpus
         */
        iterator begin() { return iterator(getpattern(0,1)); }
        //const_iterator begin() const { return data.begin(); }

        /*
         * Returns the end iterator over the corpus
         */
        iterator end() { return iterator(PatternPointer(corpus,corpussize+1)); }
        //const_iterator end() const { return data.end(); }

        /**
         * Returns an iterator starting at the given position. Correspond to
         * end() when no such position is found.
         */
        iterator find(const IndexReference & ref) {
			try {
				return iterator(getpattern(ref));
			} catch (KeyError &e) {
				return end();
			}
        }
        /**
         * Returns a const iterator starting at the given position. Correspond to
         * end() when no such position is found.
         */
        /*const_iterator find(const IndexReference & ref) const {
            return std::lower_bound(this->begin(), this->end(), IndexPattern(ref) ); //does binary search
        }*/
        
        /**
         * Does the provided position occur in the corpus? 
         */
        bool has(const IndexReference & ref) const {
			return (getpointer(ref) != NULL);
        }

        /**
         * Returns the number of tokens in the corpus
         */
        size_t size() const {
			return patternpointer.n();
		} 

        /**
         * Is the corpus empty?
         */
        bool empty() const { return (corpussize <= 1); }


        /**
         * Returns the token at the provided position. The token is returned as
         * an integer corresponding to the class in a particular class
         * encoding. Use getpattern() if you want a Pattern instance.
         * @see getpattern
         */
        unsigned int operator [](const IndexReference & ref) { 
			try {
				PatternPointer pp = getpattern(ref);
			} catch (KeyError &e) {
				throw e;
			}
			return bytestoint(pp.data,pp.bytes);
        } 

};


/************* Base abstract container for pattern storage  ********************/

/**
 * \brief Limited virtual interface to pattern stores.
 */
class PatternStoreInterface {
    public:
        /**
         * Does the pattern occur in the pattern store?
         */
        virtual bool has(const Pattern &) const =0;
        /**
         * Does the pattern occur in the pattern store?
         */
        virtual bool has(const PatternPointer &) const =0;
        /**
         * How many patterns are in the pattern store?
         */
        virtual size_t size() const =0; 
};

/**
 * \brief Abstract Pattern store class, not to be instantiated directly.
 *
 * This is an abstract class, all Pattern storage containers are derived from
 * this. 
 * @tparam ContainerType The low-level container type used (an STL container such as set/map). 
 * @tparam ReadWriteSizeType Data type for addressing, influences only the maximum number of items that can be stored (2**64) in the container, as this will be represented in the very beginning of the binary file. No reason to change this unless the container is very deeply nested in others and contains only few items.
 */
template<class ContainerType,class ReadWriteSizeType = uint64_t,class PatternType = Pattern> //,class PatternType = Pattern>
class PatternStore: public PatternStoreInterface {
    public:
        PatternStore<ContainerType,ReadWriteSizeType>() {};
        virtual ~PatternStore<ContainerType,ReadWriteSizeType>() {};
    
        virtual void insert(const PatternType & pattern)=0; //might be a noop in some implementations that require a value

        virtual bool has(const Pattern &) const =0;
        virtual bool has(const PatternPointer &) const =0;

        virtual bool erase(const PatternType &) =0;
        
        virtual size_t size() const =0; 
        virtual void reserve(size_t) =0; //might be a noop in some implementations
        
        
        typedef typename ContainerType::iterator iterator;
        typedef typename ContainerType::const_iterator const_iterator;

        virtual typename ContainerType::iterator begin()=0;
        virtual typename ContainerType::iterator end()=0;
        virtual typename ContainerType::iterator find(const Pattern & pattern)=0;
        
        virtual void write(std::ostream * out)=0;
        //virtual void read(std::istream * in, int MINTOKENS)=0;

        virtual PatternStoreInterface * getstoreinterface() {
            return (PatternStoreInterface*) this;
        }
};


/************* Abstract datatype for all kinds of maps ********************/


/**
 * \brief Abstract class for map-like pattern stores, do not instantiate directly.
 * @tparam ContainerType The low-level container type used (an STL container such as set/map). 
 * @tparam ValueType The type of Value this container stores
 * @tparam ValueHandler A handler class for this type of value
 * @tparam ReadWriteSizeType Data type for addressing, influences only the maximum number of items that can be stored (2**64) in the container, as this will be represented in the very beginning of the binary file. No reason to change this unless the container is very deeply nested in others and contains only few items.
 */
template<class ContainerType, class ValueType, class ValueHandler,class ReadWriteSizeType = uint32_t,class PatternType=Pattern>
class PatternMapStore: public PatternStore<ContainerType,ReadWriteSizeType,PatternType> { 
     protected:
        ValueHandler valuehandler;
     public:
        PatternMapStore<ContainerType,ValueType,ValueHandler,ReadWriteSizeType,PatternType>(): PatternStore<ContainerType,ReadWriteSizeType,PatternType>() {};
        virtual ~PatternMapStore<ContainerType,ValueType,ValueHandler,ReadWriteSizeType>() {};

        virtual void insert(const PatternType & pattern, ValueType & value)=0;

        virtual bool has(const Pattern &) const =0;
        virtual bool has(const PatternPointer &) const =0;

        virtual bool erase(const PatternType &) =0;

        
        virtual size_t size() const =0; 
        virtual void reserve(size_t) =0;
        
        
        virtual ValueType & operator [](const Pattern & pattern)=0;
        virtual ValueType & operator [](const PatternPointer & pattern)=0;
        
        typedef typename ContainerType::iterator iterator;
        typedef typename ContainerType::const_iterator const_iterator;

        virtual typename ContainerType::iterator begin()=0;
        virtual typename ContainerType::iterator end()=0;
        virtual typename ContainerType::iterator find(const Pattern & pattern)=0;
        virtual typename ContainerType::iterator find(const PatternPointer & pattern)=0;

        /**
         * Write the map to stream output (in binary format)
         */
        virtual void write(std::ostream * out) {
            ReadWriteSizeType s = (ReadWriteSizeType) size();
            out->write( (char*) &s, sizeof(ReadWriteSizeType));
            for (iterator iter = this->begin(); iter != this->end(); iter++) {
                PatternType p = iter->first;
                p.write(out);
                this->valuehandler.write(out, iter->second);
            }
        }
        
        /**
         * Write the map to file (in binary format)
         */
        virtual void write(std::string filename) {
            std::ofstream * out = new std::ofstream(filename.c_str());
            this->write(out);
            out->close();
            delete out;
        }


        /**
         * Read a map from input stream (in binary format)
         */
        template<class ReadValueType=ValueType, class ReadValueHandler=ValueHandler>
        void read(std::istream * in, int MINTOKENS=0, int MINLENGTH=0, int MAXLENGTH=999999, PatternStoreInterface * constrainstore = NULL, bool DONGRAMS=true, bool DOSKIPGRAMS=true, bool DOFLEXGRAMS=true, bool DORESET=false, const unsigned char classencodingversion=2, bool DEBUG=false) {
            ReadValueHandler readvaluehandler = ReadValueHandler();
            ReadWriteSizeType s; //read size:
            PatternType p;
            in->read( (char*) &s, sizeof(ReadWriteSizeType));
            reserve(s);
            if (DEBUG) std::cerr << "Reading " << s << " patterns, classencodingversion=" << classencodingversion << std::endl;
            if (MINTOKENS == -1) MINTOKENS = 0;
            for (ReadWriteSizeType i = 0; i < s; i++) {
                try {
                    p = PatternType(in, false, classencodingversion);
                } catch (std::exception &e) {
                    std::cerr << "ERROR: Exception occurred at pattern " << (i+1) << " of " << s << std::endl;
                    throw InternalError();
                }
                if (!DONGRAMS || !DOSKIPGRAMS || !DOFLEXGRAMS) {
                    const PatternCategory c = p.category();
                    if ((!DONGRAMS && c == NGRAM) || (!DOSKIPGRAMS && c == SKIPGRAM) || (!DOFLEXGRAMS && c == FLEXGRAM)) continue;
                }
                const int n = p.size();
                if (DEBUG) std::cerr << "Read pattern #" << (i+1) << ", size=" << n << ", valuehandler=" << readvaluehandler.id() << ", classencodingversion="<<(int)classencodingversion;
                ReadValueType readvalue;
                readvaluehandler.read(in, readvalue);
                if (n >= MINLENGTH && n <= MAXLENGTH)  {
                    if ((readvaluehandler.count(readvalue) >= (unsigned int) MINTOKENS) && ((constrainstore == NULL) || (constrainstore->has(p)))) {
                            ValueType * convertedvalue = NULL;
                            if (DORESET) {
                                convertedvalue = new ValueType();
                            } else {
                                readvaluehandler.convertto(&readvalue, convertedvalue); 
                                if (DEBUG) std::cerr << "...converted";
                                if (convertedvalue == NULL) {
                                    if (DEBUG) std::cerr << std::endl;
                                    std::cerr << "ERROR: Converted value yielded NULL at pattern #" << (i+1) << ", size=" << n << ", valuehandler=" << readvaluehandler.id() <<std::endl;
                                    throw InternalError();
                                }
                            }
                            if (DEBUG) std::cerr << "...adding";
                            this->insert(p,*convertedvalue);
                            if ((convertedvalue != NULL) && ((void*) convertedvalue != (void*) &readvalue)) delete convertedvalue;
                    } else if (DEBUG) {
                        if (readvaluehandler.count(readvalue) < (unsigned int) MINTOKENS) {
                            std::cerr << "...skipping because of occurrence (" << readvaluehandler.count(readvalue) << " below " << MINTOKENS;    
                        } else {
                            std::cerr << "...skipping because of constraints";    
                        }
                    }
                } else if (DEBUG) {
                  std::cerr << "...skipping because of length";
                }
                if (DEBUG) std::cerr << std::endl;
            }
        }

        /**
         * Read a map from file (in binary format)
         */
        void read(std::string filename,int MINTOKENS=0, int MINLENGTH=0, int MAXLENGTH=999999, PatternStoreInterface * constrainstore = NULL, bool DONGRAMS=true, bool DOSKIPGRAMS=true, bool DOFLEXGRAMS=true, bool DORESET = false, const unsigned char classencodingversion = 2, bool DEBUG=false) { //no templates for this one, easier on python/cython
            std::ifstream * in = new std::ifstream(filename.c_str());
            this->read<ValueType,ValueHandler>(in,MINTOKENS,MINLENGTH,MAXLENGTH,constrainstore,DONGRAMS,DOSKIPGRAMS,DOFLEXGRAMS, DORESET, classencodingversion, DEBUG);
            in->close();
            delete in;
        }

};


/************* Specific STL-like containers for patterns ********************/



typedef std::unordered_set<Pattern> t_patternset;

/**
 * \brief A pattern store in the form of an unordered set (i.e, no duplicates). 
 * Stores only patterns, no values.
 * @tparam ReadWriteSizeType The data type for addressing, determines the
 * maximum amount of patterns that can be held, only used in
 * serialisation/deserialisation
 */
template<class ReadWriteSizeType = uint32_t>
class PatternSet: public PatternStore<t_patternset,ReadWriteSizeType,Pattern> {
    protected:
        t_patternset data;
    public:

        /**
         * Empty set constructor
         */
        PatternSet<ReadWriteSizeType>(): PatternStore<t_patternset,ReadWriteSizeType>() {};


        /**
         * Constructs a pattern set from a ClassDecoder
         */
        PatternSet<ReadWriteSizeType>(const ClassDecoder & classdecoder): PatternStore<t_patternset,ReadWriteSizeType>() {
            for (ClassDecoder::const_iterator iter = classdecoder.begin(); iter != classdecoder.end(); iter++) {
                const int cls = iter->first;
                int length; //will be set by inttobytes
                unsigned char * buffer = inttobytes(cls, length);
                data.insert( Pattern(buffer, length) );
                delete buffer;
            }
        }

        /**
         * Constructs a pattern set from a ClassEncoder
         */
        PatternSet<ReadWriteSizeType>(const ClassEncoder & classencoder): PatternStore<t_patternset,ReadWriteSizeType>() {
            for (ClassEncoder::const_iterator iter = classencoder.begin(); iter != classencoder.end(); iter++) {
                const int cls = iter->second;
                int length; //will be set by inttobytes
                unsigned char * buffer = inttobytes(cls, length);
                data.insert( Pattern(buffer, length) );
                delete buffer;
            }
        }

        virtual ~PatternSet<ReadWriteSizeType>() {};

        /**
         * Add a new pattern to the set
         */
        void insert(const Pattern & pattern) {
            data.insert(pattern);
        }

        /**
         * Checks if a pattern is in the set
         */
        bool has(const Pattern & pattern) const { return data.count(pattern); }

        /**
         * Checks if a pattern is in the set
         */
        bool has(const PatternPointer & pattern) const { return data.count(pattern); }

        /**
         * Returns the number of patterns in the set
         */
        size_t size() const { return data.size(); } 
        void reserve(size_t s) { data.reserve(s); }

        typedef t_patternset::iterator iterator;
        typedef t_patternset::const_iterator const_iterator;
        
        /**
         * Returns an iterator to iterate over the set
         */
        iterator begin() { return data.begin(); }
        const_iterator begin() const { return data.begin(); }

        iterator end() { return data.end(); }
        const_iterator end() const { return data.end(); }

        /**
         * Returns an iterator to the pattern in the set or end() if no such
         * pattern was found.
         */
        iterator find(const Pattern & pattern) { return data.find(pattern); }
        const_iterator find(const Pattern & pattern) const { return data.find(pattern); }

        iterator find(const PatternPointer & pattern) { return data.find(pattern); }
        const_iterator find(const PatternPointer & pattern) const { return data.find(pattern); }

        /**
         * Removes the specified pattern from the set, returns true if successful
         */
        bool erase(const Pattern & pattern) { return data.erase(pattern); }
        iterator erase(const_iterator position) { return data.erase(position); }


        /**
         * Write the set to output stream, in binary format
         */
        void write(std::ostream * out) {
            ReadWriteSizeType s = (ReadWriteSizeType) size();
            out->write( (char*) &s, sizeof(ReadWriteSizeType));
            for (iterator iter = begin(); iter != end(); iter++) {
                Pattern p = *iter;
                p.write(out);
            }
        }

        /**
         * Read the set from input stream, in binary format
         */
        void read(std::istream * in, int MINLENGTH=0, int MAXLENGTH=999999, PatternStoreInterface * constrainstore = NULL, bool DONGRAMS=true, bool DOSKIPGRAMS=true, bool DOFLEXGRAMS=true,const unsigned char classencodingversion = 2) {
            ReadWriteSizeType s; //read size:
            in->read( (char*) &s, sizeof(ReadWriteSizeType));
            reserve(s);
            for (unsigned int i = 0; i < s; i++) {
                Pattern p = Pattern(in, false, classencodingversion);
                if (!DONGRAMS || !DOSKIPGRAMS || !DOFLEXGRAMS) {
                    const PatternCategory c = p.category();
                    if ((!DONGRAMS && c == NGRAM) || (!DOSKIPGRAMS && c == SKIPGRAM) || (!DOFLEXGRAMS && c == FLEXGRAM)) continue;
                }
                const int n = p.size();
                if ((n >= MINLENGTH && n <= MAXLENGTH) && ((constrainstore == NULL) || (constrainstore->has(p)))) {
                    insert(p);
                }
            }
        }

        /**
         * Reads a map from input stream, in binary format, but ignores the values
         * and retains only the keys for the set.
         */
        template<class ReadValueType, class ReadValueHandler=BaseValueHandler<ReadValueType>>
        void readmap(std::istream * in, int MINTOKENS=0, int MINLENGTH=0, int MAXLENGTH=999999, PatternStoreInterface * constrainstore = NULL, bool DONGRAMS=true, bool DOSKIPGRAMS=true, bool DOFLEXGRAMS=true, const unsigned char classencodingversion = 2) {
            ReadValueHandler readvaluehandler = ReadValueHandler();
            ReadWriteSizeType s; //read size:
            in->read( (char*) &s, sizeof(ReadWriteSizeType));
            reserve(s);
            //std::cerr << "Reading " << (int) s << " patterns" << std::endl;
            for (ReadWriteSizeType i = 0; i < s; i++) {
                Pattern p;
                try {
                    p = Pattern(in, false, classencodingversion);
                } catch (std::exception &e) {
                    std::cerr << "ERROR: Exception occurred at pattern " << (i+1) << " of " << s << std::endl;
                    throw InternalError();
                }
                if (!DONGRAMS || !DOSKIPGRAMS || !DOFLEXGRAMS) {
                    const PatternCategory c = p.category();
                    if ((!DONGRAMS && c == NGRAM) || (!DOSKIPGRAMS && c == SKIPGRAM) || (!DOFLEXGRAMS && c == FLEXGRAM)) continue;
                }
                const int n = p.size();
                ReadValueType readvalue;
                //std::cerr << "Read pattern: " << std::endl;
                readvaluehandler.read(in, readvalue);
                if (n >= MINLENGTH && n <= MAXLENGTH)  {
                    if ((readvaluehandler.count(readvalue) >= (unsigned int) MINTOKENS) && ((constrainstore == NULL) || (constrainstore->has(p)))) {
                        this->insert(p);
                    }
                }
            }
        }
};


typedef std::set<Pattern> t_hashorderedpatternset;


/**
 * \brief A pattern store in the form of an ordered set (i.e, no duplicates).
 * Stores only patterns, no values.
 * @tparam ReadWriteSizeType The data type for addressing, determines the
 * maximum amount of patterns that can be held, only used in
 * serialisation/deserialisation
 */
template<class ReadWriteSizeType = uint64_t>
class HashOrderedPatternSet: public PatternStore<t_hashorderedpatternset,ReadWriteSizeType> {
    protected:
        t_hashorderedpatternset data;
    public:

        HashOrderedPatternSet<ReadWriteSizeType>(): PatternStore<t_hashorderedpatternset,ReadWriteSizeType>() {};
        virtual ~HashOrderedPatternSet<ReadWriteSizeType>();

        void insert(const Pattern pattern) {
            data.insert(pattern);
        }

        bool has(const Pattern & pattern) const { return data.count(pattern); }
        bool has(const PatternPointer & pattern) const { return data.count(pattern); }
        size_t size() const { return data.size(); } 
        void reserve(size_t s) {} //noop

        typedef t_hashorderedpatternset::iterator iterator;
        typedef t_hashorderedpatternset::const_iterator const_iterator;
        
        iterator begin() { return data.begin(); }
        const_iterator begin() const { return data.begin(); }

        iterator end() { return data.end(); }
        const_iterator end() const { return data.end(); }

        iterator find(const Pattern & pattern) { return data.find(pattern); }
        const_iterator find(const Pattern & pattern) const { return data.find(pattern); }

        bool erase(const Pattern & pattern) { return data.erase(pattern); }
        iterator erase(const_iterator position) { return data.erase(position); }

        
        void write(std::ostream * out) {
            ReadWriteSizeType s = (ReadWriteSizeType) size();
            out->write( (char*) &s, sizeof(ReadWriteSizeType));
            for (iterator iter = begin(); iter != end(); iter++) {
                Pattern p = *iter;
                p.write(out);
            }
        }

        void read(std::istream * in, int MINLENGTH=0, int MAXLENGTH=999999, bool DONGRAMS=true, bool DOSKIPGRAMS=true, bool DOFLEXGRAMS=true,const unsigned char classencodingversion = 2) {
            ReadWriteSizeType s; //read size:
            in->read( (char*) &s, sizeof(ReadWriteSizeType));
            reserve(s);
            for (unsigned int i = 0; i < s; i++) {
                Pattern p = Pattern(in, false, classencodingversion);
                if (!DONGRAMS || !DOSKIPGRAMS || !DOFLEXGRAMS) {
                    const PatternCategory c = p.category();
                    if ((!DONGRAMS && c == NGRAM) || (!DOSKIPGRAMS && c == SKIPGRAM) || (!DOFLEXGRAMS && c == FLEXGRAM)) continue;
                }
                const int n = p.size();
                if (n >= MINLENGTH && n <= MAXLENGTH)  {
                    insert(p);
                }
            }
        }

};


/**
 * \brief A pattern map storing patterns and their values in a hash map (unordered_map). 
 * @tparam ValueType The type of Value this container stores
 * @tparam ValueHandler A handler class for this type of value
 * @tparam ReadWriteSizeType The data type for addressing, determines the
 * maximum amount of patterns that can be held, only used in
 * serialisation/deserialisation
 */
template<class ValueType, class ValueHandler = BaseValueHandler<ValueType>, class ReadWriteSizeType = uint64_t>
class PatternMap: public PatternMapStore<std::unordered_map<Pattern,ValueType>,ValueType,ValueHandler,ReadWriteSizeType,Pattern> {
    protected:
        std::unordered_map<Pattern, ValueType> data;
    public:
        //PatternMap(): PatternMapStore<std::unordered_map<const Pattern, ValueType>,ValueType,ValueHandler,ReadWriteSizeType>() {};
        PatternMap<ValueType,ValueHandler,ReadWriteSizeType>() {};

        void insert(const Pattern & pattern, ValueType & value) { 
            data[pattern] = value;
        }

        void insert(const Pattern & pattern) {  data[pattern] = ValueType(); } //singular insert required by PatternStore, implies 'default' ValueType, usually 0
        
        bool has(const Pattern & pattern) const { 
            return data.count(pattern);
        }
        bool has(const PatternPointer & pattern) const { return data.count(pattern); }

        size_t size() const { return data.size(); } 
        void reserve(size_t s) { data.reserve(s); }


        ValueType& operator [](const Pattern & pattern) { return data[pattern]; } 
        ValueType& operator [](const PatternPointer & pattern) { return data[pattern]; } 
        
        typedef typename std::unordered_map<Pattern,ValueType>::iterator iterator;
        typedef typename std::unordered_map<Pattern,ValueType>::const_iterator const_iterator;
        
        iterator begin() { return data.begin(); }
        const_iterator begin() const { return data.begin(); }

        iterator end() { return data.end(); }
        const_iterator end() const { return data.end(); }

        iterator find(const Pattern & pattern) { return data.find(pattern); }
        const_iterator find(const Pattern & pattern) const { return data.find(pattern); }
        iterator find(const PatternPointer & pattern) { return data.find(pattern); }
        const_iterator find(const PatternPointer & pattern) const { return data.find(pattern); }
        
        bool erase(const Pattern & pattern) { return data.erase(pattern); }
        iterator erase(const_iterator position) { return data.erase(position); }

};



template<class ValueType, class ValueHandler = BaseValueHandler<ValueType>, class ReadWriteSizeType = uint64_t>
class PatternPointerMap: public PatternMapStore<std::unordered_map<Pattern,ValueType>,ValueType,ValueHandler,ReadWriteSizeType,PatternPointer> {
    protected:
        unsigned char * corpus;
        unsigned int corpussize; //bytes
        std::map<PatternPointer, ValueType> data;
    public:
        //PatternMap(): PatternMapStore<std::unordered_map<const Pattern, ValueType>,ValueType,ValueHandler,ReadWriteSizeType>() {};
        PatternPointerMap<ValueType,ValueHandler,ReadWriteSizeType>() {};

        void insert(const PatternPointer & pattern, ValueType & value) { 
            data[pattern] = value;
        }

        void insert(const PatternPointer & pattern) {  data[pattern] = ValueType(); } //singular insert required by PatternStore, implies 'default' ValueType, usually 0
        
        bool has(const Pattern & pattern) const { 
            return data.count(pattern);
        }
        bool has(const PatternPointer & pattern) const { return data.count(pattern); }

        size_t size() const { return data.size(); } 
        void reserve(size_t s) { data.reserve(s); }


        ValueType& operator [](const Pattern & pattern) { return data[pattern]; } 
        ValueType& operator [](const PatternPointer & pattern) { return data[pattern]; } 
        
        typedef typename std::unordered_map<PatternPointer,ValueType>::iterator iterator;
        typedef typename std::unordered_map<PatternPointer,ValueType>::const_iterator const_iterator;
        
        iterator begin() { return data.begin(); }
        const_iterator begin() const { return data.begin(); }

        iterator end() { return data.end(); }
        const_iterator end() const { return data.end(); }

        iterator find(const Pattern & pattern) { return data.find(pattern); }
        const_iterator find(const Pattern & pattern) const { return data.find(pattern); }
        iterator find(const PatternPointer & pattern) { return data.find(pattern); }
        const_iterator find(const PatternPointer & pattern) const { return data.find(pattern); }
        
        bool erase(const Pattern & pattern) { return data.erase(pattern); }
        iterator erase(const_iterator position) { return data.erase(position); }
}


template<class ValueType,class ValueHandler = BaseValueHandler<ValueType>,class ReadWriteSizeType = uint64_t>
class HashOrderedPatternMap: public PatternMapStore<std::map<const Pattern,ValueType>,ValueType,ValueHandler,ReadWriteSizeType,Pattern> {
    protected:
        std::map<const Pattern, ValueType> data;
    public:
        HashOrderedPatternMap<ValueType,ValueHandler,ReadWriteSizeType>(): PatternMapStore<std::map<const Pattern, ValueType>,ValueType,ValueHandler,ReadWriteSizeType>() {};
        virtual ~HashOrderedPatternMap<ValueType,ValueHandler,ReadWriteSizeType>() {};

        void insert(const Pattern & pattern, ValueType & value) { 
            data[pattern] = value;
        }

        void insert(const Pattern & pattern) {  data[pattern] = ValueType(); } //singular insert required by PatternStore, implies 'default' ValueType

        bool has(const Pattern & pattern) const { return data.count(pattern); }
        bool has(const PatternPointer & pattern) const { return data.count(pattern); }

        size_t size() const { return data.size(); } 
        void reserve(size_t s) {} //noop

        ValueType& operator [](const Pattern & pattern) { return data[pattern]; } 
        ValueType& operator [](const PatternPointer & pattern) { return data[pattern]; } 
        
        typedef typename std::map<const Pattern,ValueType>::iterator iterator;
        typedef typename std::map<const Pattern,ValueType>::const_iterator const_iterator;
        
        iterator begin() { return data.begin(); }
        const_iterator begin() const { return data.begin(); }

        iterator end() { return data.end(); }
        const_iterator end() const { return data.end(); }

        iterator find(const Pattern & pattern) { return data.find(pattern); }
        const_iterator find(const Pattern & pattern) const { return data.find(pattern); }

        bool erase(const Pattern & pattern) { return data.erase(pattern); }
        iterator erase(const_iterator position) { return data.erase(position); }
        

};


template<class T,size_t N,int countindex = 0>
class ArrayValueHandler: public AbstractValueHandler<T> {
   public:
    const static bool indexed = false;
    virtual std::string id() { return "ArrayValueHandler"; }
    void read(std::istream * in, std::array<T,N> & a) {
        for (int i = 0; i < N; i++) {
            T v;
            in->read( (char*) &v, sizeof(T)); 
            a[i] = v;
        }
    }
    void write(std::ostream * out, std::array<T,N> & a) {
        for (int i = 0; i < N; i++) {
            T v = a[i];
            out->write( (char*) &v, sizeof(T)); 
        }
    }
    std::string tostring(std::array<T,N> & a) {
        std::string s;
        for (int i = 0; i < N; i++) {
            T v = a[i];
            if (!s.empty()) s += " ";
            s += " " + tostring(a[i]);
        }
        return s;
    }
    unsigned int count(std::array<T,N> & a) const {
        return (int) a[countindex];
    }
    void add(std::array<T,N> * value, const IndexReference & ref ) const {
        (*value)[countindex] += 1;
    }
};


/**
 * \brief A complex value handler for values that are themselves pattern stores (allows building nested maps).
 */
template<class PatternStoreType>
class PatternStoreValueHandler: public AbstractValueHandler<PatternStoreType> {
  public:
    const static bool indexed = false;
    virtual std::string id() { return "PatternStoreValueHandler"; }
    void read(std::istream * in,  PatternStoreType & value) {
        value.read(in);
    }
    void write(std::ostream * out,  PatternStoreType & value) {
        value.write(out);
    }
    virtual std::string tostring(  PatternStoreType & value) {
        std::cerr << "PatternStoreValueHandler::tostring() is not supported" << std::endl;
        throw InternalError();
    }
    unsigned int count( PatternStoreType & value) const {
        return value.size();
    }
    void add( PatternStoreType * value, const IndexReference & ref ) const {
        std::cerr << "PatternStoreValueHandler::add() is not supported" << std::endl;
        throw InternalError();
    }
};

/**
 * \brief A nested pattern map, useful for storing patterns that map to other patterns, which in turn map to values.
 * An example is phrase-translation tables in Machine Translation.
 */
template<class ValueType,class ValueHandler=BaseValueHandler<ValueType>, class NestedSizeType = uint16_t >
class AlignedPatternMap: public PatternMap< PatternMap<ValueType,ValueHandler,NestedSizeType>,PatternStoreValueHandler<PatternMap<ValueType,ValueHandler,NestedSizeType>>, uint64_t > {
    public:
        typedef PatternMap<ValueType,ValueHandler,NestedSizeType> valuetype;
        typedef typename PatternMap< PatternMap<ValueType,ValueHandler,NestedSizeType>,PatternStoreValueHandler<PatternMap<ValueType,ValueHandler,NestedSizeType>>, uint64_t >::iterator iterator;
        typedef typename PatternMap< PatternMap<ValueType,ValueHandler,NestedSizeType>,PatternStoreValueHandler<PatternMap<ValueType,ValueHandler,NestedSizeType>>, uint64_t >::const_iterator const_iterator;

};


//TODO: Implement a real Trie, conserving more memory
#endif
