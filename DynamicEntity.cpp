#include "DynamicEntity.h"
#include <string>
#include <cassert>
#include "Map.h"
#include "Sprite.h"
#include <QDebug> // TODO: remove, test
#include <QGraphicsScene>
#include "CountExpiringTimer.h"
#include <QLineF>
#include "Game.h"
#include "MoveRelativeToScreen.h"

/// Default constructor.
DynamicEntity::DynamicEntity():
    stepSize_(5),
    stepFrequency_(50),
    targetPointIndex_(0),
    rotationFrequency_(3),
    targetAngle_(0)
{
    // constructor body
    // = some defaults=
    moveTimer_ = new QTimer(this);
    rotationTimer_ = new QTimer(this);
    isPlayerControlled_ = false;
    moveBehavior_ = new MoveRealtiveToScreen();
    moveBehavior_->setEntity(this);
}

/// Causes the Entity to take 1 step closer to moving to its target point.
void DynamicEntity::moveStepAIControlled(){
    // if there are no more points to follow and entity has reached its target
    // - stop moving
    if (targetPointIndex_ >= pointsToFollow_.size() - 1 && targetPointReached()){
        stopAutomaticMovement();

        // snap
        QPointF snapPt = pointsToFollow_[targetPointIndex_];
        setCellPos(map()->pathingMap().pointToCell(snapPt));

        return;
    }

    // if there are more points to follow and entity has reached its target
    // - snap to current target
    // - set next point as target
    // - face that point
    if (targetPointIndex_ < pointsToFollow_.size() - 1 && targetPointReached()){
        // snap
        QPointF snapPt = pointsToFollow_[targetPointIndex_];
        setCellPos(map()->pathingMap().pointToCell(snapPt));

        // set next point as target
        ++targetPointIndex_;

        // face target
        rotateTo(pointsToFollow_[targetPointIndex_]);

        // recalculate path (in case pathingmap changed) TODO enable
        moveTo(pointsToFollow_.back());
    }

    // take a step closer towards the target
    stepTowardsTarget();
}

/// Causes the Entity to take 1 step towards completing it's rotation.
void DynamicEntity::rotateStep()
{
    // if it has reached its targetAngle, stop rotating
    if (abs(facingAngle() - targetAngle_) == 0 ){
        rotationTimer_->disconnect();
    }
    // other wise, rotate once towards targetAngle
    else {
        rotateTowardsTargetAngle();
    }
}

/// Returns true if the Entity is close enough to its targetPoint_.
bool DynamicEntity::targetPointReached(){
    // get a line b/w entity's pos and the targetPos
    QLineF ln(pointPos(),pointsToFollow_[targetPointIndex_]);

    // if the length of this line is less than a step size, return true
    return ln.length() < stepSize_;
}

/// Takes a step towards the targetPoint.
void DynamicEntity::stepTowardsTarget(){
    // get a line b/w the entity's pos and the target pos
    QLineF ln(pointPos(),pointsToFollow_[targetPointIndex_]);

    // set the length of this line to be the same as stepSize
    ln.setLength(stepSize_);

    // move by ln.dx and ln.dy
    double newX = pointPos().x() + ln.dx();
    double newY = pointPos().y() + ln.dy();
    QPointF newPt(newX,newY);
    setPointPos(newPt);
}

/// Rotates 1 degrees towards the target angle
void DynamicEntity::rotateTowardsTargetAngle()
{
    // rotate right if rotateRight
    if (rotateRight_){
        rotateRight();
    }

    // other wise rotate left
    else {
        rotateLeft();
    }
}

void DynamicEntity::setStepSize(int stepSize){
    stepSize_ = stepSize;
}

int DynamicEntity::stepSize(){
    return stepSize_;
}

void DynamicEntity::setStepFrequency(int to){
    stepFrequency_ = to;
}

int DynamicEntity::stepFrequency(){
    return stepFrequency_;
}

int DynamicEntity::rotationFrequency() const
{
    return rotationFrequency_;
}

void DynamicEntity::setRotationFrequency(int freq)
{
    rotationFrequency_ = freq;
}

/// Rotates the Entity 1 degrees clockwise.
void DynamicEntity::rotateRight()
{
    sprite()->setRotation(sprite()->rotation()+1);
}

/// Rotates the Entity 1 degrees counter clockwise.
void DynamicEntity::rotateLeft()
{
    sprite()->setRotation(sprite()->rotation()-1);
}

/// Rotates the Entity clockwise by the specified number of degrees.
void DynamicEntity::rotateRight(int degrees)
{
    // stop previous rotations
    stopRotating();

    // set targetAngle and direction
    targetAngle_ = facingAngle() + degrees;
    rotateRight_ = true;

    // start the timer
    connect(rotationTimer_,SIGNAL(timeout()),this,SLOT(rotateStep()));
    rotationTimer_->start(rotationFrequency());

}

/// @see Entity:rotateRight(int)
void DynamicEntity::rotateLeft(int degrees)
{
    // stop previous rotations
    stopRotating();

    // set targetAngle and direction
    targetAngle_ = facingAngle() - degrees;
    rotateRight_ = false;

    // start the timer
    connect(rotationTimer_,SIGNAL(timeout()),this,SLOT(rotateStep()));
    rotationTimer_->start(rotationFrequency_);
}

/// Rotates the Entity until it faces the specified angle. The Entity will
/// rotate in the direction that is the fastest to the specified angle. A
/// specified angle of 0 degrees is right, 90 degrees is down and so on. The
/// specified angle must be between 0-360 degrees.
void DynamicEntity::rotateTo(int angle)
{
    // make sure angle is between 0-360
    assert(angle >= 0);
    assert(angle <= 360);

    QLineF line(QPointF(0,0),QPointF(1,1));
    line.setAngle(-facingAngle());

    QLineF line2(QPointF(0,0),QPointF(1,1));
    line2.setAngle(-angle);

    double angleBWLines = line.angleTo(line2);

    if (angleBWLines < 180){
        // rotate left
        rotateLeft(angleBWLines);
    }
    else {
        // rotate right
        rotateRight(360-angleBWLines);
    }


}

/// Rotates the Entity until it faces the specified point. The Entity will
/// rotate in whatever direction is the fastest.
void DynamicEntity::rotateTo(QPointF point)
{
    QLineF line(pointPos(),point);
    int r = 360-line.angle();
    rotateTo(r);
}

/// Stops the Entity's rotating.
void DynamicEntity::stopRotating()
{
    rotationTimer_->disconnect();
}

/// Tells the Entity to move to the specified position.
///
/// Please ensure that the Entity has a "walk" animation.
void DynamicEntity::moveTo(const QPointF &pos){
    // temporarly disable entities own footing so a path can be retrieved
    // disablePathingMap();

    // get list of points from map
    std::vector<QPointF> pts = map()->pathingMap().shortestPath(pointPos(),pos);

    // stop following previous list of points
    stopAutomaticMovement();

    // follow this list of pts
    pointsToFollow_ = pts;
    targetPointIndex_ = 1; // start at 1eth not 0eth point (to prevent
                           // back movement if two quick move commands are given)
    connect(moveTimer_,SIGNAL(timeout()),this,SLOT(moveStepAIControlled()));
    moveTimer_->start(stepFrequency());

    // play walk animation
    sprite()->play("walk",-1,100);
}

/// Tells the Entity to move to the specified cell.
///
/// @see Entity::moveTo(const QPointF&)
void DynamicEntity::moveTo(const Node &cell)
{
    moveTo(map()->cellToPoint(cell));
}

/// Tells the Entity to move up by one cell of the Map's PathingMap.
void DynamicEntity::moveUp(){
    // get the location of one cell up
    Node c = map()->pathingMap().pointToCell(pointPos());
    c.setY(c.y()-1);
    QPointF pt = map()->pathingMap().cellToPoint(c);

    // move to this point
    moveTo(pt);
}

/// Tells the Entity to move up down one cell of the Map's PathingMap.
void DynamicEntity::moveDown(){
    // get the location of one cell up
    Node c = map()->pathingMap().pointToCell(pointPos());
    c.setY(c.y()+1);
    QPointF pt = map()->pathingMap().cellToPoint(c);

    // move to this point
    moveTo(pt);
}

/// Tells the Entity to move left by one cell of the Map's PathingMap.
void DynamicEntity::moveLeft(){
    // get the location of one cell up
    Node c = map()->pathingMap().pointToCell(pointPos());
    c.setX(c.x()-1);
    QPointF pt = map()->pathingMap().cellToPoint(c);

    // move to this point
    moveTo(pt);
}

/// Tells the Entity to move right by one cell of the Map's PathingMap.
void DynamicEntity::moveRight(){
    // get the location of one cell up
    Node c = map()->pathingMap().pointToCell(pointPos());
    c.setX(c.x()+1);
    QPointF pt = map()->pathingMap().cellToPoint(c);

    // move to this point
    moveTo(pt);
}

/// Tells the Entity to stop moving.
void DynamicEntity::stopAutomaticMovement(){
    moveTimer_->disconnect();

    // play stand animation
    sprite()->play("stand",-1,100);
}

/// Sets the movement behavior of the Entity when being controlled by a player.
void DynamicEntity::setPlayerControlledMoveBehavior(PlayerControlledMoveBehavior *behavior)
{
    moveBehavior_ = behavior;
}

/// Returns true if the Entity is controlled by a player.
bool DynamicEntity::isPlayerControlled()
{
    return isPlayerControlled_;
}

/// Sets whether the Entity is controlled by the player or not.
///
/// If the Entity is controlled by a player, it will move in response to
/// keyboard/mouse input. If the Entity is not controlled by a player, it will
/// move in response to its own thinking.
void DynamicEntity::setPlayerControlled(bool tf)
{
    isPlayerControlled_ = tf;

    // if player controlled
    if (tf == true){
        // connect moveTimer_ with movePlayerControlled()
        moveTimer_->disconnect();
        connect(moveTimer_,SIGNAL(timeout()),this,SLOT(moveStepPlayerControlled()));
        moveTimer_->start(stepFrequency());
    }
    else{
        // connect moveTimer_ with moveAIControlled()
        moveTimer_->disconnect();
    }
}


/// Moves the Entity 1 step closer to its movement in response to keys.
void DynamicEntity::moveStepPlayerControlled()
{
    // delegate to moveBehavior
    moveBehavior_->moveStep();
}