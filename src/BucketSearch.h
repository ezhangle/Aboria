/*

Copyright (c) 2005-2016, University of Oxford.
All rights reserved.

University of Oxford means the Chancellor, Masters and Scholars of the
University of Oxford, having an administrative office at Wellington
Square, Oxford OX1 2JD, UK.

This file is part of Aboria.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
 * Neither the name of the University of Oxford nor the names of its
   contributors may be used to endorse or promote products derived from this
   software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/


#ifndef BUCKETSEARCH_H_
#define BUCKETSEARCH_H_

#include <boost/iterator/iterator_facade.hpp>
#include "Vector.h"
#include "Constants.h"
#include "Log.h"
#include <vector>
#include <iostream>
#include <set>


namespace Aboria {

/// a constant indicating an empty cell
const int CELL_EMPTY = -1;

/// \brief Implements neighbourhood searching using a bucket search algorithm, dividing
/// the domain into constant size "buckets".
///
/// This class implements neighbourhood searching using a bucket search algorithm. The 
/// domain is first divided up into a regular grid of constant size "buckets", either by
/// using the class constructor to initialise the domain extents, bucket size etc., or by 
/// using the reset() member function to reset these parameters.
///
/// After the buckets are created, a set of 3D points can be assigned to their respective buckets
/// using the embed_points() member function. After this, neighbourhood queries around
/// a given point can be performed using find_broadphase_neighbours(), which returns a const 
/// iterator to all the points in the same bucket or surrounding buckets of the given point.
///
template<typename T, typename F>
class BucketSearch {
public:

    /// A const iterator to a set of neighbouring points. This iterator implements
    /// a STL forward iterator type
	class const_iterator
	{
	 public:
		  typedef const std::tuple<const typename T::value_type&,const Vect3d&>* pointer;
		  typedef std::forward_iterator_tag iterator_category;
		  typedef const std::tuple<const typename T::value_type&,const Vect3d&> value_type;
		  typedef const std::tuple<const typename T::value_type&,const Vect3d&> reference;
		  typedef std::ptrdiff_t difference_type;


          /// This constructor is used to return an end iterator
		  const_iterator()
	      : m_node(),my_index(-1),self(false) {
			  cell_empty.push_back(CELL_EMPTY);
			  m_node = cell_empty.begin();
		  }

//		  const_iterator(const const_iterator& arg):
//			  bucket_sort(arg.bucket_sort),
//			  m_node(arg.m_node),
//			  cell_i(arg.cell_i),
//			  my_index(arg.my_index),
//			  self(arg.self),
//			  centre(arg.centre),
//			  dx(arg.dx),
//			  cell_empty(arg.cell_empty),
//			  surrounding_cell_offset_end(arg.surrounding_cell_offset_end),
//			  surrounding_cell_offset_i(arg.surrounding_cell_offset_i) {}

	    void go_to_next_candidate() {
	    	if (*m_node != CELL_EMPTY) {
	    		m_node = bucket_sort->linked_list.begin() + *m_node;
	    		//std::cout << "going to new particle *mnode = "<<*m_node<<std::endl;
	    	}
	    	while ((*m_node == CELL_EMPTY) && (surrounding_cell_offset_i != surrounding_cell_offset_end)) {
	    		if (self && (surrounding_cell_offset_i == surrounding_cell_offset_end-1)) {
	    			m_node = cell_i;
	    			while (*m_node != my_index) {
	    				m_node = bucket_sort->linked_list.begin() + *m_node;
	    			}
	    		} else {
		    		//std::cout << "going to new_cell with offset = "<<*surrounding_cell_offset_i<<std::endl;
	    			m_node = cell_i + *surrounding_cell_offset_i;
	    		}
	    		surrounding_cell_offset_i++;
	    	}
//	    	if (surrounding_cell_offset_i == surrounding_cell_offset_end) {
//	    		std::cout <<"finished all cells"<<std::endl;
//	    	} else {
//	    		std::cout <<"*m_node = "<<*m_node<<std::endl;
//	    	}

	    }

        /// this constructor is used to start the iterator at the head of a bucket 
        /// list
        /// \param bucket_sort a pointer to the parent BucketSearch class
        /// \param centre the neighbourhood query point
        /// \param my_index (optional) the index of the query point
        /// \param self (optional) if true then assume that the user is finding
        /// all point pairs within a single container and so don't return
        /// each point pair twice (i.e. only 1/2 the normal neighbour checking required)
	    explicit const_iterator(const BucketSearch* bucket_sort,
	    		const Vect3d centre,
	    		const int my_index = -1, const bool self = false)
	    : bucket_sort(bucket_sort),
	      my_index(my_index),self(self),
	      centre(centre) {
	    	cell_empty.push_back(CELL_EMPTY);
	    	m_node = cell_empty.begin();
//	    	if (((centre.array() < bucket_sort->low.array()).any()) ||
//	    			((centre.array() >= bucket_sort->high.array()).any())) {
//	    		return;
//	    	}
	    	cell_i = bucket_sort->cells.begin() + bucket_sort->find_cell_index(centre);
	    	surrounding_cell_offset_i = bucket_sort->surrounding_cell_offsets.begin();
	    	if (self) {
	    		surrounding_cell_offset_end = surrounding_cell_offset_i
	    						+ (bucket_sort->surrounding_cell_offsets.size()-1)/2 + 1;
	    	} else {
	    		//std::cout << "num offsets = "<<bucket_sort->surrounding_cell_offsets.size()<<std::endl;
	    		surrounding_cell_offset_end = bucket_sort->surrounding_cell_offsets.end();
	    	}

	    	increment();

	    }

	    reference operator *() {
	    	return dereference();
	    }
	    reference operator ->() {
	    	return dereference();
	    }
	    const_iterator& operator++() {
	    	increment();
	    	return *this;
	   }
	    const_iterator operator++(int) {
	    	const_iterator tmp(*this);
	    	operator++();
	    	return tmp;
	    }
	    size_t operator-(const_iterator start) const {
	    	size_t count = 0;
	    	while (start != *this) {
	    		start++;
	    		count++;
	    	}
	    	return count;
	    }
	    inline bool operator==(const const_iterator& rhs) {
	    	return equal(rhs);
	    }
	    inline bool operator!=(const const_iterator& rhs){
	    	return !operator==(rhs);
	    }

	 private:
	    friend class boost::iterator_core_access;

	    bool equal(const_iterator const& other) const {
	    	//std::cout <<" testing equal *m_node = "<<*m_node<<" other.m_node = "<<*(other.m_node)<<std::endl;
	        return *m_node == *(other.m_node);
	    }

	    void increment() {
	    	//std::cout <<" increment "<<std::endl;
	    	go_to_next_candidate();
	    	while (*m_node != CELL_EMPTY) {
	    		const Vect3d p = bucket_sort->return_vect3d(bucket_sort->begin_iterator[*m_node]);

	    		dx = centre-bucket_sort->correct_position_for_periodicity(centre, p);
	    		//std::cout << "testing candidate with position "<<p<<" and dx = "<<dx<<" relative to centre = "<<centre<<std::endl;
	    		//if (dx.squaredNorm() <= radius2) {
	    		if ((std::abs(dx[0]) < bucket_sort->max_interaction_radius) &&
	    				(std::abs(dx[1]) < bucket_sort->max_interaction_radius) &&
	    				(std::abs(dx[2]) < bucket_sort->max_interaction_radius)) {

	    	    	//std::cout << "found candidate with position"<<p<<std::endl;
	    	    	//std::cout << "max interact rad = "<<bucket_sort->max_interaction_radius<<std::endl;
	    	    	break;
	    		} else {
	    			go_to_next_candidate();
	    		}

	    	}
	    }


	    reference dereference() const
	    { return std::tie(bucket_sort->begin_iterator[*m_node],dx); }



	    const BucketSearch* bucket_sort;
	    std::vector<int>::const_iterator m_node;
	    std::vector<int>::const_iterator cell_i;
	    //Value* const linked_list;
	    int my_index;
	    bool self;
	    Vect3d centre;
	    Vect3d dx;
	    std::vector<int> cell_empty;
	//    std::vector<Vect3d>::const_iterator positions;
	//    std::vector<Value>::const_iterator linked_list;
	    std::vector<int>::const_iterator surrounding_cell_offset_i,surrounding_cell_offset_end;
	};

    /// This constructor sets the domain extents and their periodicity 
    /// \param low the lower extent of the search domain
    /// \param high the upper extent of the search domain
    /// \param periodic a boolean vector indicating wether each dimension
    /// is periodic or not.
    /// \param return_vect3d a function that takes a value_type representing
    /// a particle in 3D space and returns its position
	BucketSearch(Vect3d low, Vect3d high, Vect3b periodic, F return_vect3d):
		return_vect3d(return_vect3d),
		low(low),high(high),domain_size(high-low),periodic(periodic),
		empty_cell(CELL_EMPTY) {
		LOG(2,"Creating bucketsort data structure with lower corner = "<<low<<" and upper corner = "<<high);
		const double dx = (high-low).maxCoeff()/10.0;
		reset(low, high, dx,periodic);
	}

    /// resets the domain extents, periodicity and bucket size
    /// \param low the lower extent of the search domain
    /// \param high the upper extent of the search domain
    /// \param _max_interaction_radius the side length of each bucket
    /// \param periodic a boolean vector indicating wether each dimension
	void reset(const Vect3d& low, const Vect3d& high, double _max_interaction_radius, const Vect3b& periodic);

	inline const Vect3d& get_low() const {return low;}
	inline const Vect3d& get_high() const {return high;}
	inline const Vect3b& get_periodic() const {return periodic;}
	inline const double get_lengthscale() const {return max_interaction_radius;}

    /// embed a set of points into the buckets, assigning each 3D point into the bucket
    /// that contains that point. Any points already assigned to the buckets are 
    /// removed. 
    /// \param begin_iterator an iterator to the beginning of the set of points
    /// \param end_iterator an iterator to the end of the set of points
    /// \see embed_points_incremental() 
	void embed_points(const T begin_iterator, const T end_iterator);

    /// embed a set of points into the buckets, assigning each 3D point into the bucket
    /// that contains that point. The primary difference to embed_points() is that any
    /// existing points are kept
    /// \param begin_iterator an iterator to the beginning of the set of points
    /// \param end_iterator an iterator to the end of the set of points
    /// \see embed_points()
	void embed_points_incremental(const T begin_iterator, const T end_iterator);

    /// This class keeps track of the searchable particles through begin and end
    /// iterators to a STL-compatible container. If these need to be updated
    /// (for example if the memory allocation is changed) this function can be 
    /// used to update them. Note that if the ordering of the particles changes
    /// then the particles will need to be embedded again
	void update_begin_and_end(const T begin_iterator, const T end_iterator);


    /// add a singular point to the buckets. Note that it is assumed that this is 
    /// also added to the end of the container holding the points
    /// \param point_to_add an iterator pointing to the point to add
	void add_point(const T point_to_add);

    /// remove a point from the buckets, so it will not occur in neighbourhood
    /// queries. If you wish to delete a point entirely (i.e. in the original
    /// point container), you need to use delete_point()
    /// \param untrack_iterator an iterator pointing to the point to remove. Note 
    /// that this iterator should be between begin_iterator and end_iterator
	void untrack_point(const T untrack_iterator);

    /// delete a point from the buckets and pop it off the linked list, 
    /// so it will not occur in neighbourhood queries. Note that this point must be
    /// at (end_iterator-1),
    /// \param untrack_iterator an iterator pointing to the point to delete. Note 
    /// should be  == (end_iterator-1)
	void delete_point(const T untrack_iterator);


    /// update the bucket for a specified point (if it has changed position sufficiently)
    /// \param untrack_iterator an iterator pointing to the point to remove. Note 
    /// that this iterator should be between begin_iterator and end_iterator
	void update_point(const T untrack_iterator);


    /// If a point is copied from \p copy_from_iterator to \p copy_to_iterator in the 
    /// point container, then this function can be used to still keep the bucketsearch
    /// data structures correct
	void copy_points(const T copy_to_iterator, const T copy_from_iterator);


    /// return a const forward iterator to all the points in the neighbourhood of \p r. If 
    /// this function is being used to find all the point pairs within the same point container, then
    /// a naive looping through and using find_broadphase_neighbours() will find each pair twice. 
    /// This can be avoided by setting self=true and supplying the index of each point with my_index
    const_iterator find_broadphase_neighbours(const Vect3d& r, const int my_index, const bool self) const;

    /// return an end() iterator to compare against the result of find_broadphase_neighbours in order
    /// to determine the end of the neighbour list
	const_iterator end() const;

    /// in periodic domains a point pair can have multiple separations, but what is normally required for
    /// neighbourhood searches is the minimum separation. This function corrects \p to_correct_r in relation
    /// to \p source_r so that it is closest to \p source r
	Vect3d correct_position_for_periodicity(const Vect3d& source_r, const Vect3d& to_correct_r) const;

    /// this function corrects the position of \p to_correct_r so that it is within the extents of a periodic
    /// domain. If the domain is not periodic the Vect3d is unchanged
	Vect3d correct_position_for_periodicity(const Vect3d& to_correct_r) const;

private:

	inline int vect_to_index(const Vect3i& vect) const {
		return vect[0] * num_cells_along_yz + vect[1] * num_cells_along_axes[2] + vect[2];
	}
	inline int find_cell_index(const Vect3d &r) const {
		const Vect3i celli = floor(((r-low)*inv_cell_size)) + Vect3i(1,1,1);
		ASSERT((celli[0] > 0) && (celli[0] < num_cells_along_axes[0]-1), "position is outside of x-range "<<r);
		ASSERT((celli[1] > 0) && (celli[1] < num_cells_along_axes[1]-1), "position is outside of y-range "<<r);
		ASSERT((celli[2] > 0) && (celli[2] < num_cells_along_axes[2]-1), "position is outside of z-range "<<r);
		//std::cout << " looking in cell " << celli <<" out of total cells " << num_cells_along_axes << " at position " << r<< std::endl;
		return vect_to_index(celli);
	}

	T begin_iterator,end_iterator;
	const F return_vect3d;
    std::vector<int> cells;
    std::vector<std::vector<int> > ghosting_indices_pb;
    std::vector<std::pair<int,int> > ghosting_indices_cb;
    std::vector<int> dirty_cells;
    bool use_dirty_cells;
	std::vector<int> linked_list,linked_list_reverse;
	std::vector<int> neighbr_list;
	Vect3d low,high,domain_size;
	Vect3b periodic;
	Vect3d cell_size,inv_cell_size;
	Vect3i num_cells_along_axes;
	int num_cells_along_yz;
	double max_interaction_radius;
	std::vector<int> surrounding_cell_offsets;
	const int empty_cell;
};

template<typename T, typename F>
void BucketSearch<T,F>::embed_points_incremental(const T _begin_iterator, const T _end_iterator) {
	//TODO: embed_points_incremental doesn't work. And what if particles are deleted since last update?
	begin_iterator = _begin_iterator;
	end_iterator = _end_iterator;
	const unsigned int n = std::distance(begin_iterator,end_iterator);
	//std::cout <<"embedding "<<n<<" particles"<<std::endl;
	const int old_size = linked_list.size();
	linked_list.assign(n, CELL_EMPTY);
	linked_list_reverse.assign(n, CELL_EMPTY);
	dirty_cells.assign(n,CELL_EMPTY);

	int i = 0;
	for (auto it = begin_iterator; it != end_iterator; ++it, ++i) {
		const int celli = find_cell_index(return_vect3d(*it));
		if (i < old_size) {
			if (celli == dirty_cells[i]) continue;

			// Remove from old cell
			const int forwardi = linked_list[i];
			const int backwardsi = linked_list_reverse[i];

			if (forwardi != CELL_EMPTY) linked_list_reverse[forwardi] = backwardsi;
			if (backwardsi != CELL_EMPTY) {
				linked_list[backwardsi] = forwardi;
			} else {
				const int old_celli = dirty_cells[i];
				ASSERT(cells[old_celli]==i,"inconsistant cells data structures!");
				cells[old_celli] = forwardi;
				for (int j: ghosting_indices_pb[old_celli]) {
					cells[j] = forwardi;
				}
			}
		}

		// Insert into new cell
		cells[celli] = i;
		dirty_cells[i] = celli;
		const int cell_entry = cells[celli];
		linked_list[i] = cell_entry;
		linked_list_reverse[i] = CELL_EMPTY;
		if (cell_entry != CELL_EMPTY) linked_list_reverse[cell_entry] = i;

		// Insert into ghosted cells
		for (int j: ghosting_indices_pb[celli]) {
			cells[j] = i;
		}
	}
}

template<typename T, typename F>
void BucketSearch<T,F>::update_begin_and_end(const T _begin_iterator, const T _end_iterator) {
	begin_iterator = _begin_iterator;
	end_iterator = _end_iterator;
}


template<typename T, typename F>
void BucketSearch<T,F>::embed_points(const T _begin_iterator, const T _end_iterator) {
	begin_iterator = _begin_iterator;
	end_iterator = _end_iterator;
	const unsigned int n = std::distance(begin_iterator,end_iterator);
	//std::cout <<"embedding "<<n<<" particles"<<std::endl;


	/*
	 * clear head of linked lists (cells)
	 */
	if (use_dirty_cells) {
		if (dirty_cells.size()<cells.size()) {
			for (int i: dirty_cells) {
				cells[i] = CELL_EMPTY;
				for (int j: ghosting_indices_pb[i]) {
					cells[j] = CELL_EMPTY;
				}
			}
		} else {
			cells.assign(cells.size(), CELL_EMPTY);
		}
	}
	use_dirty_cells = true;

	linked_list.assign(n, CELL_EMPTY);
	linked_list_reverse.assign(n, CELL_EMPTY);
	dirty_cells.assign(n,CELL_EMPTY);
	//const bool particle_based = dirty_cells.size() < cells.size();
	const bool particle_based = true; //TODO: fix cell_based neighbour ghosting list
	const bool use_dirty = n < cells.size();
	int i = 0;
	for (auto it = begin_iterator; it != end_iterator; ++it, ++i) {
		const int celli = find_cell_index(return_vect3d(*it));
		const int cell_entry = cells[celli];

		// Insert into own cell
		cells[celli] = i;
		dirty_cells[i] = celli;
		linked_list[i] = cell_entry;
		linked_list_reverse[i] = CELL_EMPTY;
		if (cell_entry != CELL_EMPTY) linked_list_reverse[cell_entry] = i;

		// Insert into ghosted cells
		if (particle_based) {
			for (int j: ghosting_indices_pb[celli]) {
				cells[j] = i;
			}
		}
	}


	if (!particle_based) {
		for (std::vector<std::pair<int,int> >::iterator index_pair = ghosting_indices_cb.begin(); index_pair != ghosting_indices_cb.end(); ++index_pair) {
			//BOOST_FOREACH(std::pair<int,int> index_pair, ghosting_indices) {
			cells[index_pair->first] = cells[index_pair->second];
		}
	}
}

template<typename T, typename F>
void BucketSearch<T,F>::add_point(const T point_to_add_iterator) {
	linked_list.push_back(CELL_EMPTY);
	linked_list_reverse.push_back(CELL_EMPTY);
	dirty_cells.push_back(CELL_EMPTY);

	int i = linked_list.size()-1;
	const bool particle_based = true;

	const int celli = find_cell_index(return_vect3d(*point_to_add_iterator));
	const int cell_entry = cells[celli];

	// Insert into own cell
	cells[celli] = i;
	dirty_cells[i] = celli;
	linked_list[i] = cell_entry;
	linked_list_reverse[i] = CELL_EMPTY;
	if (cell_entry != CELL_EMPTY) linked_list_reverse[cell_entry] = i;

	// Insert into ghosted cells
	if (particle_based) {
		for (int j: ghosting_indices_pb[celli]) {
			cells[j] = i;
		}
	}
}
template<typename T, typename F>
void BucketSearch<T,F>::delete_point(const T untrack_iterator) {
	const unsigned int i = std::distance(begin_iterator,untrack_iterator);
	CHECK(i==linked_list.size()-1,"point to delete not at end of sequence");

	const int forwardi = linked_list[i];
	const int backwardsi = linked_list_reverse[i];

	if (forwardi != CELL_EMPTY) linked_list_reverse[forwardi] = backwardsi;
	if (backwardsi != CELL_EMPTY) {
		linked_list[backwardsi] = forwardi;
	} else {
		//const int celli = find_cell_index(return_vect3d(*untrack_iterator));
		const int celli = dirty_cells[i];
		ASSERT(cells[celli]==i,"inconsistant cells data structures!");
		cells[celli] = forwardi;
	}

	linked_list.pop_back();
	linked_list_reverse.pop_back();
	dirty_cells.pop_back();
}

template<typename T, typename F>
void BucketSearch<T,F>::update_point(const T update_iterator) {
	const unsigned int i = std::distance(begin_iterator,update_iterator);
	const bool particle_based = true;

	const int forwardi = linked_list[i];
	const int backwardsi = linked_list_reverse[i];

	if (forwardi != CELL_EMPTY) linked_list_reverse[forwardi] = backwardsi;
	if (backwardsi != CELL_EMPTY) {
		linked_list[backwardsi] = forwardi;
	} else {
		//const int celli = find_cell_index(return_vect3d(*update_iterator));
		const int celli = dirty_cells[i];
		ASSERT(cells[celli]==i,"inconsistant cells data structures!");
		cells[celli] = forwardi;
	}

	const int celli = find_cell_index(return_vect3d(*update_iterator));
	const int cell_entry = cells[celli];

	// Insert into own cell
	cells[celli] = i;
	dirty_cells[i] = celli;
	linked_list[i] = cell_entry;
	linked_list_reverse[i] = CELL_EMPTY;
	if (cell_entry != CELL_EMPTY) linked_list_reverse[cell_entry] = i;

	// Insert into ghosted cells
	if (particle_based) {
		for (int j: ghosting_indices_pb[celli]) {
			cells[j] = i;
		}
	}

}

template<typename T, typename F>
void BucketSearch<T,F>::untrack_point(const T untrack_iterator) {
	const unsigned int i = std::distance(begin_iterator,untrack_iterator);
	ASSERT((i>=0) && (i<linked_list.size()),"invalid untrack index");

	const int forwardi = linked_list[i];
	const int backwardsi = linked_list_reverse[i];

	if (forwardi != CELL_EMPTY) linked_list_reverse[forwardi] = backwardsi;
	if (backwardsi != CELL_EMPTY) {
		linked_list[backwardsi] = forwardi;
	} else {
		const int celli = dirty_cells[i];
		//const int celli = find_cell_index(return_vect3d(*untrack_iterator));
		ASSERT(cells[celli]==i,"inconsistant cells data structures!");
		cells[celli] = forwardi;
	}
}



template<typename T, typename F>
void BucketSearch<T,F>::copy_points(const T copy_to_iterator, const T copy_from_iterator) {
	const unsigned int toi = std::distance(begin_iterator,copy_to_iterator);
	const unsigned int fromi = std::distance(begin_iterator,copy_from_iterator);
	ASSERT((toi>=0) && (toi<linked_list.size()),"invalid copy iterator");
	ASSERT((fromi>=0) && (fromi<linked_list.size()),"invalid copy iterator");
	if (toi==fromi) return;


	const int forwardi = linked_list[fromi];

	linked_list[toi] = forwardi;
	linked_list_reverse[toi] = fromi;
	linked_list[fromi] = toi;
	linked_list_reverse[forwardi] = toi;
	dirty_cells[toi] = dirty_cells[fromi];
}

template<typename T, typename F>
void BucketSearch<T,F>::reset(const Vect3d& _low, const Vect3d& _high, double _max_interaction_radius, const Vect3b& _periodic) {
	LOG(2,"Resetting bucketsort data structure:");
	LOG(2,"\tMax interaction radius = "<<_max_interaction_radius);
	high = _high;
	low = _low;
	domain_size = high-low;
	periodic = _periodic;
	LOG(2,"\tPeriodic = "<<periodic);

	max_interaction_radius = _max_interaction_radius;
	Vect3i num_cells_without_ghost = (high-low)/max_interaction_radius;
	Vect3b search(true,true,true);
	for (int i = 0; i < 3; ++i) {
		if (num_cells_without_ghost[i]==0) {
			LOG(2,"\tNote: Dimension "<<i<<" has no length, setting cell side equal to interaction radius.");
			LOG(1,"\tNote: Dimension "<<i<<" has no length, turning off neighbour search in this dimension.");
			search[i] = false;
            const Vect3d middle_of_range = 0.5*(high[i]-low[i]);
			high[i] = low[i] + max_interaction_radius;
			num_cells_without_ghost[i] = 1;
		} else if (periodic[i]) {
            CHECK(num_cells_without_ghost[i]>2,"Number of cells in dimension "<<i<<" must be greater than 2. Set max_interaction_radius < "<<(high[i]-low[i])/3);
        }
	}
	num_cells_along_axes = num_cells_without_ghost + Vect3i(3,3,3);
	LOG(2,"\tNumber of cells along each axis = "<<num_cells_along_axes);
	cell_size = (high-low)/(num_cells_without_ghost);
	LOG(2,"\tCell sizes along each axis = "<<cell_size);
	inv_cell_size = Vect3d(1,1,1)/cell_size;
	num_cells_along_yz = num_cells_along_axes[2]*num_cells_along_axes[1];
	const unsigned int num_cells = num_cells_along_axes.prod();
	cells.assign(num_cells, CELL_EMPTY);
	use_dirty_cells = false;
	//TODO: assumed 3d
	surrounding_cell_offsets.clear();

	for (int i = -1; i < 2; ++i) {
		if ((i != 0) && !search[0]) continue;
		for (int j = -1; j < 2; ++j) {
			if ((j != 0) && !search[1]) continue;
			for (int k = -1; k < 2; ++k) {
				if ((k != 0) && !search[2]) continue;
				surrounding_cell_offsets.push_back(vect_to_index(Vect3i(i,j,k)));
			}
		}
	}


	ghosting_indices_pb.assign(num_cells, std::vector<int>());
	ghosting_indices_cb.clear();
	for (int i = 0; i < NDIM; ++i) {
		if (!periodic[i] || !search[i]) continue;
		int j,k;
		switch (i) {
			case 0:
				j = 1;
				k = 2;
				break;
			case 1:
				j = 0;
				k = 2;
				break;
			case 2:
				j = 0;
				k = 1;
				break;
			default:
				break;
		}

		Vect3i tmp;
		const int n = num_cells_along_axes[i];
		for (int jj = 0; jj < num_cells_along_axes[j]-1; ++jj) {
			tmp[j] = jj;
			for (int kk = 0; kk < num_cells_along_axes[k]-1; ++kk) {
				tmp[k] = kk;
				tmp[i] = n-3;
				const int index_from1 = vect_to_index(tmp);
				ASSERT(index_from1 < num_cells,"from index 1 ("<<index_from1<<") is greater than total number of cells ("<<num_cells<<"). tmp = "<<tmp);
				tmp[i] = 0;
				const int index_to1 = vect_to_index(tmp);
				ASSERT(index_to1 < num_cells,"to index 1 ("<<index_to1<<") is greater than total number of cells ("<<num_cells<<"). tmp = "<<tmp);
				ghosting_indices_pb[index_from1].push_back(index_to1);
				//ghosting_indices_cb.push_back(std::pair<int,int>(index_to1,index_from1));
				tmp[i] = 1;
				const int index_from2 = vect_to_index(tmp);
				ASSERT(index_from2 < num_cells,"from index 2 ("<<index_from2<<") is greater than total number of cells ("<<num_cells<<"). tmp = "<<tmp);

				tmp[i] = n-2;
				const int index_to2 = vect_to_index(tmp);
				ghosting_indices_pb[index_from2].push_back(index_to2);
				ASSERT(index_to2 < num_cells,"to index 2 ("<<index_to2<<") is greater than total number of cells ("<<num_cells<<"). tmp = "<<tmp);
				//ghosting_indices_cb.push_back(std::pair<int,int>(index_to2,index_from2));
			}
		}
	}
	/*
	 * collapse redirections
	 */
	for (int i = 0; i < num_cells; ++i) {
		std::set<int> ghosting_cells;
		for (int j: ghosting_indices_pb[i]) {
			ghosting_cells.insert(j);
			for (int k: ghosting_indices_pb[j]) {
				ghosting_cells.insert(k);
				for (int m: ghosting_indices_pb[k]) {
					ghosting_cells.insert(m);
				}
			}
		}
		ghosting_indices_pb[i].resize(ghosting_cells.size());
		std::copy(ghosting_cells.begin(),ghosting_cells.end(),ghosting_indices_pb[i].begin());
	}
}
template<typename T, typename F>
typename BucketSearch<T,F>::const_iterator BucketSearch<T,F>::find_broadphase_neighbours(const Vect3d& r,const int my_index, const bool self) const {
	return const_iterator(this,correct_position_for_periodicity(r),my_index,self);
}
template<typename T, typename F>
typename BucketSearch<T,F>::const_iterator BucketSearch<T,F>::end() const {
	return const_iterator();
}
template<typename T, typename F>
Vect3d BucketSearch<T,F>::correct_position_for_periodicity(const Vect3d& source_r, const Vect3d& to_correct_r) const {
	Vect3d corrected_r = to_correct_r - source_r;
	for (int i = 0; i < NDIM; ++i) {
		if (!periodic[i]) continue;
		if (corrected_r[i] > domain_size[i]/2.0) corrected_r[i] -= domain_size[i];
		else if (corrected_r[i] < -domain_size[i]/2.0) corrected_r[i] += domain_size[i];
	}
	return corrected_r + source_r;
}

template<typename T, typename F>
Vect3d BucketSearch<T,F>::correct_position_for_periodicity(const Vect3d& to_correct_r) const {
	Vect3d corrected_r = to_correct_r;
	for (int i = 0; i < NDIM; ++i) {
		if (!periodic[i]) continue;
		while (corrected_r[i] >= high[i]) corrected_r[i] -= domain_size[i];
		while (corrected_r[i] < low[i]) corrected_r[i] += domain_size[i];
	}
	return corrected_r;
}



}

#endif /* BUCKETSEARCH_H_ */
