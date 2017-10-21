
#ifndef SRC_SEED_H_
#define SRC_SEED_H_

#include <memory>

class Cell;
class Plant;
class Traits;

class Seed
{
    protected:
       Cell* cell;

    public:
       std::unique_ptr<Traits> traits;

       double mass;
       double pEstab;
       int age;
       bool toBeRemoved;

       Seed(const std::unique_ptr<Traits> & t, Cell* cell, ITV_mode itv, double aSD);
       Seed(const std::unique_ptr<Traits> & t, Cell* cell, const double estab, ITV_mode itv, double aSD);

       Cell* getCell() { return cell; }

       inline static bool GetSeedRemove(const std::unique_ptr<Seed> & s) {
           return s->toBeRemoved;
       };
};

#endif
