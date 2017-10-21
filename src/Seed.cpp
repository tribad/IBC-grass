
#include <cassert>
#include <iostream>
#include <vector>

#include "itv_mode.h"
#include "Seed.h"
#include "Traits.h"
#include "Environment.h"
#include "Plant.h"

using namespace std;

//-----------------------------------------------------------------------------

/*
 * Constructor for normal reproduction
 */
Seed::Seed(const unique_ptr<Traits> & t, Cell* _cell, ITV_mode itv, double aSD) :
		cell(NULL),
		age(0), toBeRemoved(false)
{
    traits = make_unique<Traits>(*t);

    if (itv == on) {
        traits->varyTraits(aSD);
	}

	pEstab = traits->pEstab;
	mass = traits->seedMass;

	assert(this->cell == NULL);
	this->cell = _cell;
}

//-----------------------------------------------------------------------------

/*
 * Constructor for initial establishment (with germination pre-set)
 */
Seed::Seed(const unique_ptr<Traits> & t, Cell*_cell, double new_estab, ITV_mode itv, double aSD) :
		cell(NULL),
		age(0), toBeRemoved(false)
{
    traits = make_unique<Traits>(*t);

    if (itv == on) {
        traits->varyTraits(aSD);
	}

	pEstab = new_estab;
	mass = traits->seedMass;

	assert(this->cell == NULL);
	this->cell = _cell;
}
