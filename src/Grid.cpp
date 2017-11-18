#include <algorithm>
#include <iostream>
#include <cassert>
#include <memory>
#include <math.h>

#include "itv_mode.h"
#include "Grid.h"
#include "Environment.h"
#include "RandomGenerator.h"
#include "Output.h"
#include "IBC-grass.h"

using namespace std;

//---------------------------------------------------------------------------

Grid::Grid()
{

}

//---------------------------------------------------------------------------

Grid::~Grid()
{
    for (int i = 0; i < getGridArea(); ++i)
    {
        Cell* cell = CellList[i];
        delete cell;
    }
    delete[] CellList;

    ZOIBase.clear();

    Plant::staticID = 0;
    Genet::staticID = 0;
}

//-----------------------------------------------------------------------------

void Grid::CellsInit()
{
    int index;
    int SideCells = GridSize;
    CellList = new Cell*[SideCells * SideCells];

    for (int x = 0; x < SideCells; x++)
    {
        for (int y = 0; y < SideCells; y++)
        {
            index = x * SideCells + y;
            Cell* cell = 0;

            if ((BelowCompMode == sym) && (AboveCompMode == asympart)) {
                switch (stabilization) {
                case version1:
                    cell = new CellAsymPartSymV1(x, y);
                    break;
                case version2:
                    cell = new CellAsymPartSymV2(x, y);
                    break;
                case version3:
                    cell = new CellAsymPartSymV3(x, y);
                    break;
                default:
                    cerr << "Invalid stabilization mode. Exiting\n";
                    exit(0);
                }
            }

            if (cell != 0) {
                cell->SetResource(meanARes, meanBRes);
                CellList[index] = cell;
            } else {
                cerr << "Something went wrong. \n";
            }
        }
    }

    ZOIBase = vector<int>(getGridArea(), 0);

    for (unsigned int i = 0; i < ZOIBase.size(); i++)
    {
        ZOIBase[i] = i;
    }

    sort(ZOIBase.begin(), ZOIBase.end(), CompareIndexRel);
}

//-----------------------------------------------------------------------------

void Grid::PlantLoop()
{
    for (auto const& p : PlantList)
    {
        if (ITV == on)
            assert(p->traits->myTraitType == Traits::individualized);

        if (!p->isDead)
        {
            p->Grow(week);

            if (p->traits->clonal)
            {
                DisperseRamets(p);
                p->SpacerGrow();
            }

//			if (CEnvir::week >= p->Traits->DispWeek)
            if (week > p->traits->dispersalWeek)
            {
                DisperseSeeds(p);
            }

            p->Kill(backgroundMortality, week);
        }
        else
        {
            p->DecomposeDead(litterDecomp);
        }
    }
}

//-----------------------------------------------------------------------------
/**
 lognormal dispersal kernel
 Each Seed is dispersed after an log-normal dispersal kernel with mean and sd
 given by plant traits. The dispersal direction has no prevalence.
 */
void getTargetCell(int& xx, int& yy, const float mean, const float sd)
{
    double sigma = std::sqrt(std::log((sd / mean) * (sd / mean) + 1));
    double mu = std::log(mean) - 0.5 * sigma;
    double dist = exp(rng.getGaussian(mu, sigma));

    // direction uniformly distributed
    double direction = 2 * Pi * rng.get01();
    xx = round(xx + cos(direction) * dist);
    yy = round(yy + sin(direction) * dist);
}

//-----------------------------------------------------------------------------
/**
 * Disperses the seeds produced by a plant when seeds are to be released.
 * Each Seed is dispersed after an log-normal dispersal kernel in function getTargetCell().
 */
void Grid::DisperseSeeds(const std::shared_ptr<Plant> & plant)
{
    int px = plant->getCell()->x;
    int py = plant->getCell()->y;
    int n = plant->ConvertReproMassToSeeds();

    for (int i = 0; i < n; ++i)
    {
        int x = px; // remember parent's position
        int y = py;

        // lognormal dispersal kernel. This function changes X & Y by reference!
        getTargetCell(x, y,
                plant->traits->dispersalDist * 100,  // meters -> cm
                plant->traits->dispersalDist * 100); // mean = std (simple assumption)

        Torus(x, y); // recalculates position for torus

        Cell* cell = CellList[x * GridSize + y];

        cell->SeedBankList.push_back(make_unique<Seed>(createTraitSetFromPftType(plant->traits->PFT_ID), cell, ITV, ITVsd));
    }
}

//---------------------------------------------------------------------------

void Grid::DisperseRamets(const std::shared_ptr<Plant> & p)
{
    assert(p->traits->clonal);

    if (p->GetNRamets() == 1)
    {
        double distance = std::abs(rng.getGaussian(p->traits->meanSpacerlength, p->traits->sdSpacerlength));

        // uniformly distributed direction
        double direction = 2 * Pi * rng.get01();
        int x = round(p->getCell()->x + cos(direction) * distance);
        int y = round(p->getCell()->y + sin(direction) * distance);

        // periodic boundary condition
        Torus(x, y);

        // save distance and direction in the plant
        std::shared_ptr<Plant> Spacer = make_shared<Plant>(x, y, p, ITV);
        Spacer->spacerLengthToGrow = distance; // This spacer now has to grow to get to its new cell
        p->growingSpacerList.push_back(Spacer);
    }
}

//--------------------------------------------------------------------------
/**
 * This function calculates ZOI of all plants on grid.
 * Each grid-cell gets a list of plants influencing the above- (alive and dead plants) and
 * belowground (alive plants only) layers.
 */
void Grid::CoverCells()
{
    for (auto const& plant : PlantList)
    {
        double Ashoot = plant->Area_shoot();
        plant->Ash_disc = floor(Ashoot) + 1;

        double Aroot = plant->Area_root();
        plant->Art_disc = floor(Aroot) + 1;

        double Amax = max(Ashoot, Aroot);

        for (int a = 0; a < Amax; a++)
        {
            int x = plant->getCell()->x
                    + ZOIBase[a] / GridSize
                    - GridSize / 2;
            int y = plant->getCell()->y
                    + ZOIBase[a] % GridSize
                    - GridSize / 2;

            Torus(x, y);

            Cell* cell = CellList[x * GridSize + y];

            // Aboveground
            if (a < Ashoot)
            {
                // dead plants still shade others
                cell->AbovePlantList.push_back(plant);
                cell->PftNIndA[plant->pft()]++;
            }

            // Belowground
            if (a < Aroot)
            {
                // dead plants do not compete for below ground resource
                if (!plant->isDead)
                {
                    cell->BelowPlantList.push_back(plant);
                    cell->PftNIndB[plant->pft()]++;
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
/**
 * Resets all weekly variables of individual cells and plants (only in PlantList)
 */
void Grid::ResetWeeklyVariables()
{
    for (int i = 0; i < getGridArea(); ++i)
    {
        Cell* cell = CellList[i];
        cell->weeklyReset();
    }

    for (auto const& p : PlantList)
    {
        p->weeklyReset();
    }
}

//---------------------------------------------------------------------------
/**
 * Distributes local resources according to local competition
 * and shares them between connected ramets of clonal genets.
 */
void Grid::DistributeResource()
{
    for (int i = 0; i < getGridArea(); ++i)
    {
        Cell* cell = CellList[i];

        cell->AboveComp();
        cell->BelowComp();
    }

    shareResources();
}

//----------------------------------------------------------------------------
/**
 * Resource sharing between connected ramets
 */
void Grid::shareResources()
{
    for (auto const& Genet : GenetList)
    {
        if (Genet->RametList.size() > 1) // A ramet cannot share with itself
        {
            auto ramet = Genet->RametList.front().lock();
            assert(ramet);
            assert(ramet->traits->clonal);

            if (ramet->traits->resourceShare)
            {
                Genet->ResshareA();
                Genet->ResshareB();
            }
        }
    }
}

//-----------------------------------------------------------------------------

void Grid::EstablishmentLottery()
{
    /*
     * Explicit use of indexes rather than iterators because RametEstab adds to PlantList,
     * thereby sometimes invalidating them. Also, RametEstab takes a shared_ptr rather
     * than a reference to a shared_ptr, thus avoiding invalidation of the reference
     */

    std::vector< shared_ptr<Plant>>::size_type original_size = PlantList.size();
    for (std::vector< shared_ptr<Plant> >::size_type i = 0; i < original_size; ++i)
    {
        auto const& plant = PlantList[i];

        if (plant->traits->clonal && !plant->isDead)
        {
            establishRamets(plant);
        }
    }

    int w = Environment::week;
    if ( !( (w >= 1 && w < 5) || (w > 21 && w <= 25) ) ) // establishment is only between weeks 1-4 and 21-25
//	if ( !( (w >= 1 && w < 5) || (w >= 21 && w < 25) ) ) // establishment is only between weeks 1-4 and 21-25
    {
        return;
    }

    for (int i = 0; i < getGridArea(); ++i)
    {
        Cell* cell = CellList[i];

        if (!cell->AbovePlantList.empty() || cell->SeedBankList.empty() || cell->occupied)
        {
            continue;
        }

        double sumSeedMass = cell->Germinate();

        if ( Environment::AreSame(sumSeedMass, 0) ) // No seeds germinated
        {
            continue;
        }

        double n = rng.get01() * sumSeedMass;
        for (auto const& itr : cell->SeedlingList)
        {
            n -= itr->mass;
            if (n <= 0)
            {
                establishSeedlings(itr);
                break;
            }
        }
        cell->SeedlingList.clear();
    }
}

//-----------------------------------------------------------------------------

void Grid::establishSeedlings(const std::unique_ptr<Seed> & seed)
{
    shared_ptr<Plant> p = make_shared<Plant>(seed, ITV);

    shared_ptr<Genet> genet = make_shared<Genet>();
    GenetList.push_back(genet);

    genet->RametList.push_back(p);
    p->setGenet(genet);

    PlantList.push_back(p);
}

//-----------------------------------------------------------------------------

void Grid::establishRamets(const std::shared_ptr<Plant> plant)
{
    auto spacer_itr = plant->growingSpacerList.begin();

    while (spacer_itr != plant->growingSpacerList.end())
    {
        const auto& spacer = *spacer_itr;

        if (spacer->spacerLengthToGrow > 0) // This spacer still has to grow more, keep it.
        {
            spacer_itr++;
            continue;
        }

        Cell* cell = CellList[spacer->x * GridSize + spacer->y];

        if (!cell->occupied)
        {
            if (rng.get01() < rametEstab)
            {
                // This spacer successfully establishes into a ramet (CPlant) of a genet
                auto Genet = spacer->getGenet().lock();
                assert(Genet);

                Genet->RametList.push_back(spacer);
                spacer->setCell(cell);
                PlantList.push_back(spacer);
            }

            // Regardless of establishment success, the iterator is removed from growingSpacerList
            spacer_itr = plant->growingSpacerList.erase(spacer_itr);
        }
        else
        {
            if (Environment::week == Environment::WeeksPerYear)
            {
                // It is winter so this spacer dies over the winter
                spacer_itr = plant->growingSpacerList.erase(spacer_itr);
            }
            else
            {
                // This spacer will find a nearby cell; keep it
                int _x, _y;
                do
                {
                    _x = rng.getUniformInt(5) - 2;
                    _y = rng.getUniformInt(5) - 2;
                } while (_x == 0 && _y == 0);

                int x = std::round(spacer->x + _x);
                int y = std::round(spacer->y + _y);

                Torus(x, y);

                spacer->x = x;
                spacer->y = y;
                spacer->spacerLengthToGrow = Distance(_x, _y, 0, 0);

                spacer_itr++;
            }

        }
    }
}

//-----------------------------------------------------------------------------

void Grid::SeedMortalityAge()
{
    for (int i = 0; i < getGridArea(); ++i)
    {
        Cell* cell = CellList[i];

        for (auto const& seed : cell->SeedBankList)
        {
            if (seed->age >= seed->traits->dormancy)
            {
                seed->toBeRemoved = true;
            }
        }
        cell->RemoveSeeds();
    }
}

//-----------------------------------------------------------------------------

void Grid::Disturb()
{
    Grid::below_biomass_history.push_back(GetTotalBelowMass());

    if (rng.get01() < AbvGrazProb) {
        GrazingAbvGr();
    }

    if (rng.get01() < BelGrazProb) {
        GrazingBelGr();
    }

    if (NCut > 0) {
        switch (NCut) {
        case 1:
            if (Environment::week == 22)
                Cutting(CutHeight);
            break;
        case 2:
            if (Environment::week == 22 || Environment::week == 10)
                Cutting(CutHeight);
            break;
        case 3:
            if (Environment::week == 22 || Environment::week == 10 || Environment::week == 16)
                Cutting(CutHeight);
            break;
        default:
            cerr << "CGrid::Disturb() - wrong input";
            exit(1);
        }
    }
}

//-----------------------------------------------------------------------------

void Grid::RunCatastrophicDisturbance()
{
    for (auto const& p : PlantList)
    {
        if (p->isDead)
            continue;

        if (rng.get01() < CatastrophicPlantMortality)
        {
            p->isDead = true;
        }
    }

//	Cutting(5);
}

//-----------------------------------------------------------------------------
/**
 The plants on the whole grid are grazed according to
 their relative grazing susceptibility until the given "proportion of removal"
 is reached or the grid is completely grazed.
 (Aboveground mass that is ungrazable - see Schwinning and Parsons (1999):
 15,3 g/m�  * 1.6641 m� = 25.5 g)
 */
void Grid::GrazingAbvGr()
{
    double ResidualMass = MassUngrazable * getGridArea() * 0.0001;

    double TotalAboveMass = GetTotalAboveMass();

    double MaxMassRemove = min(TotalAboveMass - ResidualMass, TotalAboveMass * AbvPropRemoved);
    double MassRemoved = 0;

    while (MassRemoved < MaxMassRemove)
    {
        auto p = *std::max_element(PlantList.begin(), PlantList.end(),
                        [](const shared_ptr<Plant> & a, const shared_ptr<Plant> & b)
                        {
                            return Plant::getPalatability(a) < Plant::getPalatability(b);
                        });

        double max_palatability = Plant::getPalatability(p);

        std::shuffle( PlantList.begin(), PlantList.end(), rng.getRNG() );

        for (auto const& plant : PlantList)
        {
            if (MassRemoved >= MaxMassRemove)
            {
                break;
            }

            if (plant->isDead)
            {
                continue;
            }

            double grazProb = Plant::getPalatability(plant) / max_palatability;

            if (rng.get01() < grazProb)
            {
                MassRemoved += plant->RemoveShootMass(BiteSize);
            }
        }
    }
}

//-----------------------------------------------------------------------------
/**
 * Cutting of all plants on the patch to a uniform height.
 */
void Grid::Cutting(double cut_height)
{
    for (auto const& i : PlantList)
    {
        if (i->getHeight() > cut_height)
        {
            double biomass_at_height = i->getBiomassAtHeight(cut_height);

            i->mShoot = biomass_at_height;
            i->mRepro = 0.0;
        }
    }
}

//-----------------------------------------------------------------------------

void Grid::GrazingBelGr()
{
    assert(!Grid::below_biomass_history.empty());

    // Total living root biomass
    double bt = accumulate(PlantList.begin(), PlantList.end(), 0,
                    [] (double s, const shared_ptr<Plant>& p)
                    {
                        if ( !p->isDead )
                        {
                            s = s + p->mRoot;
                        }
                        return s;
                    });

    const double alpha = BelGrazAlpha;

    std::vector<double> rolling_mean;
    vector<double>::size_type historySize = BelGrazHistorySize; // in Weeks
    if (Grid::below_biomass_history.size() > historySize)
    {
        rolling_mean = std::vector<double>(Grid::below_biomass_history.end() - historySize, Grid::below_biomass_history.end());
    }
    else
    {
        rolling_mean = std::vector<double>(Grid::below_biomass_history.begin(), Grid::below_biomass_history.end());
    }

    double fn_o = BelGrazPerc * ( accumulate(rolling_mean.begin(), rolling_mean.end(), 0) / rolling_mean.size() );

    // Functional response
    if (bt - fn_o < bt * BelGrazResidualPerc)
    {
        fn_o = bt - bt * BelGrazResidualPerc;
    }

    output.BlwgrdGrazingPressure.push_back(fn_o);
    output.ContemporaneousRootmassHistory.push_back(bt);

    double fn = fn_o;
    double t_br = 0; // total biomass removed
    while (ceil(t_br) < fn_o)
    {
        double bite = 0;
        for (auto const& p : PlantList)
        {
            if (!p->isDead)
            {
                bite += pow(p->mRoot / bt, alpha) * fn;
            }
        }
        bite = fn / bite;

        double br = 0; // Biomass removed this iteration
        double leftovers = 0; // When a plant is eaten to death, this is the overshoot from the algorithm
        for (auto const& p : PlantList)
        {
            if (p->isDead)
            {
                continue;
            }

            double biomass_to_remove = pow(p->mRoot / bt, alpha) * fn * bite;

            if (biomass_to_remove >= p->mRoot)
            {
                leftovers = leftovers + (biomass_to_remove - p->mRoot);
                br = br + p->mRoot;
                p->mRoot = 0;
                p->isDead = true;
            }
            else
            {
                p->RemoveRootMass(biomass_to_remove);
                br = br + biomass_to_remove;
            }
        }

        t_br = t_br + br;
        bt = bt - br;
        fn = leftovers;

        assert(bt >= 0);
    }
}

//-----------------------------------------------------------------------------

void Grid::RemovePlants()
{
    // Delete the CPlant shared_pointers
    PlantList.erase(
            std::remove_if(PlantList.begin(), PlantList.end(),
                    [] (const shared_ptr<Plant> & p)
                    {
                        if ( Plant::GetPlantRemove(p) )
                        {
                            p->getCell()->occupied = false;
                            return true;
                        }
                        return false;
                    }),
                    PlantList.end());


    // Clear out dead pointers in the AllRametsList held by each genet
    auto eraseExpiredRamet = []( const std::weak_ptr<Plant> & r )
    {
        if (r.expired())
        {
            return true;
        }
        return false;
    };

    std::for_each(GenetList.begin(), GenetList.end(),
            [eraseExpiredRamet] (std::shared_ptr<Genet> const& g)
            {
                auto& r = g->RametList;
                r.erase(std::remove_if(r.begin(), r.end(), eraseExpiredRamet), r.end());
            });

    // Delete any empty genets
    GenetList.erase(
            std::remove_if(GenetList.begin(), GenetList.end(),
                    [] (const shared_ptr<Genet> & g)
                    {
                        if (g->RametList.empty())
                        {
                            return true;
                        }
                        return false;
                    }),
                    GenetList.end());
}

//-----------------------------------------------------------------------------

void Grid::Winter()
{
    RemovePlants();
    for (auto const& p : PlantList)
    {
        p->WinterLoss(winterDieback);
    }
}

//-----------------------------------------------------------------------------

void Grid::SeedMortalityWinter()
{
    for (int i = 0; i < getGridArea(); ++i)
    {
        Cell* cell = CellList[i];
        for (auto const& seed : cell->SeedBankList)
        {
            if (rng.get01() < seedMortality)
            {
                seed->toBeRemoved = true;
            }
            else
            {
                ++seed->age;
            }
        }

        cell->RemoveSeeds();
    }
}

//-----------------------------------------------------------------------------
/**
 * Set a number of randomly distributed clonal Seeds of a specific trait-combination on the grid.
 */
void Grid::InitSeeds(string PFT_ID, const int n, const double estab)
{
    for (int i = 0; i < n; ++i)
    {
        int x = rng.getUniformInt(GridSize);
        int y = rng.getUniformInt(GridSize);

        Cell* cell = CellList[x * GridSize + y];

        cell->SeedBankList.push_back(make_unique<Seed>(createTraitSetFromPftType(PFT_ID), cell, estab, ITV, ITVsd));
    }
}

//---------------------------------------------------------------------------

/**
 * Weekly sets cell's resources. Above- and belowground variation during the year.
 */
void Grid::SetCellResources()
{
    int gweek = Environment::week;

    for (int i = 0; i < getGridArea(); ++i) {
        Cell* cell = CellList[i];
        cell->SetResource(
                max(0.0,
                        (-1.0) * Aampl
                                * cos(
                                        2.0 * Pi * gweek
                                                / double(Environment::WeeksPerYear))
                                + meanARes),
                max(0.0,
                        Bampl
                                * sin(
                                        2.0 * Pi * gweek
                                                / double(Environment::WeeksPerYear))
                                + meanBRes));
    }
}

//-----------------------------------------------------------------------------

double Distance(const double xx, const double yy, const double x, const double y)
{
    return sqrt((xx - x) * (xx - x) + (yy - y) * (yy - y));
}

//-----------------------------------------------------------------------------

bool CompareIndexRel(const int i1, const int i2)
{
    const int n = GridSize;

    return Distance(i1 / n, i1 % n, n / 2, n / 2) < Distance(i2 / n, i2 % n, n / 2, n / 2);
}

//---------------------------------------------------------------------------
/*
 * Accounts for the gridspace being torus
 */
void Torus(int& xx, int& yy)
{
    xx %= GridSize;
    if (xx < 0)
    {
        xx += GridSize;
    }

    yy %= GridSize;
    if (yy < 0)
    {
        yy += GridSize;
    }
}

//---------------------------------------------------------------------------

double Grid::GetTotalAboveMass()
{
    double above_mass = 0;
    for (auto const& p : PlantList)
    {
        if (p->isDead)
        {
            continue;
        }

        above_mass += p->mShoot + p->mRepro;
    }
    return above_mass;
}

//---------------------------------------------------------------------------

double Grid::GetTotalBelowMass()
{
    double below_mass = 0;
    for (auto const& p : PlantList)
    {
        if (p->isDead)
        {
            continue;
        }

        below_mass += p->mRoot;
    }
    return below_mass;
}

double Grid::GetTotalAboveComp()
{
    double above_comp = 0;
    for (int i = 0; i < getGridArea(); i++)
    {
        Cell* cell = CellList[i];
        above_comp += cell->aComp_weekly;
    }

    return above_comp;
}

double Grid::GetTotalBelowComp()
{
    double below_comp = 0;
    for (int i = 0; i < getGridArea(); i++)
    {
        Cell* cell = CellList[i];
        below_comp += cell->bComp_weekly;
    }

    return below_comp;
}

//-----------------------------------------------------------------------------

int Grid::GetNclonalPlants()
{
    int NClonalPlants = 0;
    for (auto const& g : GenetList)
    {
        bool hasLivingRamet = false;

        for (auto const& r_ptr : g->RametList)
        {
            auto const& r = r_ptr.lock();
            if (!r->isDead)
            {
                hasLivingRamet = true;
                break;
            }
        }

        if (hasLivingRamet)
        {
            ++NClonalPlants;
        }
    }
    return NClonalPlants;
}

//-----------------------------------------------------------------------------

int Grid::GetNPlants() //count non-clonal plants
{
    int NPlants = 0;
    for (auto const& p : PlantList)
    {
        //only if its a non-clonal plant
        if (!p->traits->clonal && !p->isDead)
        {
            NPlants++;
        }
    }
    return NPlants;
}

//-----------------------------------------------------------------------------

int Grid::GetNSeeds()
{
    int seedCount = 0;
    for (int i = 0; i < getGridArea(); ++i)
    {
        Cell* cell = CellList[i];
        seedCount = seedCount + int(cell->SeedBankList.size());
    }

    return seedCount;
}

