#include "RangedWeapon.h"

RangedWeapon::RangedWeapon()
{
}

/// Returns the point at which projectiles will be spawn.
/// This point is in local coordinates (relative to the RangedWeapon itself).
QPointF RangedWeapon::projectileSpawnPoint()
{
    return this->projectileSpawnPoint_;
}

void RangedWeapon::setProjectileSpawnPoint(QPointF point)
{
    this->projectileSpawnPoint_ = point;
}

/// Resets the projectile spawn point to be at the very center of the RangedWeapon.
void RangedWeapon::resetProjectileSpawnPoint()
{
    QPointF pt;
    pt.setX(length()/2);
    pt.setY(width()/2);
    setProjectileSpawnPoint(pt);
}