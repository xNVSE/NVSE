#include <stdexcept>

#include "ArrayVar.h"

ArrayVarElementContainer::ArrayVarElementContainer(bool isArray)
{
	this->isArray_ = isArray;
	if (isArray)
	{
		this->array_ = std::make_shared<std::vector<ArrayElement>>();
	}
	else
	{
		this->map_ = std::make_unique<std::map<ArrayKey, ArrayElement>>();
	}
}

ArrayVarElementContainer::ArrayVarElementContainer() = default;

ArrayElement& ArrayVarElementContainer::operator[](ArrayKey i) const
{
	if (isArray_) [[likely]]
	{
		const std::size_t index = i.Key().num;
		if (index >= array_->size())
		{
			array_->push_back(ArrayElement());
			return array_->back();
		}
		return (*array_)[index];
	}
	return (*map_)[i];
}

ArrayVarElementContainer::iterator::iterator() = default;

ArrayVarElementContainer::iterator::iterator(const std::map<ArrayKey, ArrayElement>::iterator iter)
{
	this->isArray_ = false;
	this->mapIter_ = iter;
}

ArrayVarElementContainer::iterator::iterator(std::vector<ArrayElement>::iterator iter,
                                             std::shared_ptr<std::vector<ArrayElement>> vectorRef)
{
	this->isArray_ = true;
	this->arrIter_ = iter;
	this->vectorRef_ = std::move(vectorRef);
}

void ArrayVarElementContainer::iterator::operator++()
{
	if (isArray_) [[likely]]
	{
		++arrIter_;
	}
	else
	{
		++mapIter_;
	}
}

void ArrayVarElementContainer::iterator::operator--()
{
	if (isArray_) [[likely]]
	{
		--arrIter_;
	}
	else
	{
		--mapIter_;
	}
}

bool ArrayVarElementContainer::iterator::operator!=(const iterator& other) const
{
	if (other.isArray_ != this->isArray_)
	{
		return true;
	}
	if (isArray_) [[likely]]
	{
		return other.arrIter_ != this->arrIter_;
	}
	return other.mapIter_ != this->mapIter_;
}

ArrayKey ArrayVarElementContainer::iterator::first() const
{
	if (isArray_) [[likely]]
	{
		return ArrayKey(arrIter_ - vectorRef_->begin());
	}
	return mapIter_->first;
}

ArrayElement& ArrayVarElementContainer::iterator::second() const
{
	if (isArray_) [[likely]]
	{
		return *arrIter_;
	}
	return mapIter_->second;
}

ArrayVarElementContainer::iterator ArrayVarElementContainer::find(ArrayKey key) const
{
	if (isArray_) [[likely]]
	{
		std::size_t index = key.Key().num;
		if (index >= array_->size())
		{
			return iterator(array_->end(), array_);
		}
		return iterator(array_->begin() + key.Key().num, array_);
	}
	return iterator(map_->find(key));
}

ArrayVarElementContainer::iterator ArrayVarElementContainer::begin() const
{
	if (isArray_) [[likely]]
	{
		return iterator(array_->begin(), array_);
	}
	return iterator(map_->begin());
}

ArrayVarElementContainer::iterator ArrayVarElementContainer::end() const
{
	if (isArray_) [[likely]]
	{
		return iterator(array_->end(), array_);
	}
	return iterator(map_->end());
}

std::size_t ArrayVarElementContainer::size() const
{
	if (isArray_) [[likely]]
	{
		return array_->size();
	}
	return map_->size();
}

std::size_t ArrayVarElementContainer::erase(const ArrayKey key) const
{
	if (isArray_) [[likely]]
	{
		array_->erase(array_->begin() + key.Key().num);
		return 1;
	}
	return map_->erase(key);
}

std::size_t ArrayVarElementContainer::erase(const ArrayKey low, const ArrayKey high) const
{
	if (!isArray_)
	{
		throw std::invalid_argument("Attempt to erase range of elements from map");
	}
	if (high.Key().num > array_->size() || low.Key().num > array_->size())
	{
		throw std::out_of_range("Attempt to erase out of range array var");
	}
	array_->erase(array_->begin() + low.Key().num, array_->begin() + high.Key().num);
	return high.Key().num - low.Key().num;
}
