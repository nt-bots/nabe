#include "nav.h"

#include "thirdparty/source-sdk-stubs/nav_area.h"

HidingSpot* GetHidingSpotByID(unsigned int id)
{
	for (auto& hiding_spot : TheHidingSpotList) {
		if (hiding_spot->GetID() == id) {
			return hiding_spot;
		}
	}
	return nullptr;
}

NavConnect::operator unsigned int() const
{
	return (area != nullptr) ? area->GetID() : 0;
}

SpotOrder::~SpotOrder()
{
	if (spot) {
		delete spot;
	}
}