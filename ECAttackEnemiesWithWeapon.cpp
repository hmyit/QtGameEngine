#include "ECAttackEnemiesWithWeapon.h"

#include "ECChaseEnemies.h"
#include <cassert>
#include "Slot.h"
#include "Weapon.h"
#include "WeaponSlot.h"
#include "Utilities.h"

ECAttackEnemiesWithWeapon::ECAttackEnemiesWithWeapon(Entity *entity):
    EntityController(entity),
    controllerChaseEnemies_(new ECChaseEnemies(entity))
{
    // listen to chaser
    ECChaseEnemies* ce = controllerChaseEnemies_.get();
    connect(ce, &ECChaseEnemies::entityChaseContinued,this,&ECAttackEnemiesWithWeapon::onEnemyChaseContinued);
    connect(ce,&ECChaseEnemies::entityChaseStarted,this,&ECAttackEnemiesWithWeapon::onEnemyChaseContinued);
}

/// Executed whenever the controlled entity takes one step closer to chased entity.
/// Will see if close enough to use weapon. If so, will use it.
void ECAttackEnemiesWithWeapon::onEnemyChaseContinued(Entity *entityChased, double distance)
{
    // TODO only attack if close to range of weapon
    // pick first weapon in controlled enities slots and use it to attack chased entity
    for (Slot* slot: entityControlled()->getSlots()){
        WeaponSlot* asWS = dynamic_cast<WeaponSlot*>(slot);
        if (asWS){
            if (asWS->isFilled()){
                Weapon* weapon = dynamic_cast<Weapon*>(asWS->item());
                if (weapon){
                    if (distance < weapon->castRange()){
                        weapon->attack(entityChased->pos());
                        emit attacked(entityChased);
                    }
                }
            }
        }
    }
}
