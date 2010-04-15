/*
-----------------------------------------------------------------------------
Copyright (c) 2010 Nigel Atkinson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

#include <animation.h>
#include <animationeventdata.h>

#include <OgreRoot.h>
#include <OgreEntity.h>
#include <OgreSceneNode.h>
#include <OgreAnimation.h>

MeshAnimation::MeshAnimation( Ogre::Entity* ent, Ogre::String name )
        : mFadingIn(false), mFadingOut(false), mRemoveWhenFinished(true)
{
    mState = ent->getAnimationState( name );
    assert(mState);

    setFadeSpeed();
}

void MeshAnimation::addTime( Ogre::Real timeSinceLastFrame )
{
    if( mState->getEnabled() )
    {
        mState->addTime( timeSinceLastFrame );

        if( mFadingIn )
        {
            Ogre::Real weight = mState->getWeight();

            weight += timeSinceLastFrame * mFadeSpeed;

            if( weight > 1.0 )
            {
                mFadingIn = false;
                weight = 1.0;
            }

            mState->setWeight( weight );
        }

        if( mFadingOut )
        {
            Ogre::Real weight = mState->getWeight();

            weight -= timeSinceLastFrame * mFadeSpeed;

            if( weight < 0.0 )
            {
                mFadingOut = false;
                weight = 0.0;
            }

            mState->setWeight( weight );
        }
    }
}

void MeshAnimation::setWeighting( Ogre::Real weighting )
{
    mState->setWeight( weighting );
}

void MeshAnimation::setFadeSpeed( Ogre::Real percentWeightPerSecond /*= DEFAULT_FADE_SPEED */ )
{
    mFadeSpeed = percentWeightPerSecond;
}

void MeshAnimation::fadeIn()
{
    mFadingIn = true;
    mFadingOut = false;
}

void MeshAnimation::fadeOut()
{
    mFadingIn = false;
    mFadingOut = true;}

bool MeshAnimation::isFadingIn()
{
    return mFadingIn;
}

bool MeshAnimation::isFadingOut()
{
    return mFadingOut;
}

void MeshAnimation::setLoop( bool loop )
{
    mState->setLoop( loop );
}

void MeshAnimation::stop()
{
    mState->setEnabled( false );
}

void MeshAnimation::start()
{
    mState->setEnabled( true );
}

bool MeshAnimation::isFinished()
{
    return mState->getLength() < mState->getTimePosition();
}

bool MeshAnimation::shouldRemoveWhenFinished()
{
    return mRemoveWhenFinished;
}

void MeshAnimation::setRemoveWhenFinished( bool shouldRemoveWhenFinished )
{
    mRemoveWhenFinished = shouldRemoveWhenFinished;
}

MovementAnimation::MovementAnimation( Ogre::SceneNode* nodeToMove, Ogre::Vector3 destination, Ogre::Real movementSpeed )
{
    mNode = nodeToMove;
    mSpeed = movementSpeed;
    mDestination = destination;
}

void MovementAnimation::addTime( Ogre::Real timeSinceLastFrame )
{
    if( mMoving )
    {
        Ogre::Vector3 position = mNode->getPosition();

        // We work out direction each time, as the node may have been re-positioned by other code.
        Ogre::Vector3 direction = mDestination - position;

        direction.normalise();

        position += direction * mSpeed * timeSinceLastFrame;

        mNode->setPosition( position );
    }
}

void MovementAnimation::stop()
{
    mMoving = true;
}

void MovementAnimation::start()
{
    mMoving = false;
}

bool MovementAnimation::isFinished()
{
    return mNode->getPosition() == mDestination;
}

template<> AnimationManager *Ogre::Singleton<AnimationManager>::ms_Singleton=0;

AnimationManager::AnimationManager() :
        finishEvent( Event::hash( "EVT_ANIMATIONFINISHED" ) ),
        timeSinceLastFrame(0)
{
}

AnimationManager::~AnimationManager()
{
}

void AnimationManager::initailise()
{
    Ogre::Root *root = Ogre::Root::getSingletonPtr();
    root->addFrameListener( this );
}

void AnimationManager::shutdown()
{
    Ogre::Root *root = Ogre::Root::getSingletonPtr();
    root->removeFrameListener( this );
}

AnimationManager* AnimationManager::getSingletonPtr()
{
    return ms_Singleton;
}

AnimationManager& AnimationManager::getSingleton()
{
    assert( ms_Singleton );
    return *ms_Singleton;
}

void AnimationManager::addAnimation( AnimationPtr animation )
{
    animations.push_back( animation );
}

void AnimationManager::removeAnimation( AnimationPtr animation )
{
    std::vector<AnimationPtr>::iterator i;

    i = std::find( animations.begin(), animations.end(), animation );

    if( i != animations.end() )
        animations.erase( i );
}

void AnimationManager::update()
{
    std::vector<AnimationPtr>::iterator i;

    i = animations.begin();
    while( i != animations.end() )
    {
        (*i)->addTime( timeSinceLastFrame );

        if( (*i)->isFinished() )
        {
            boost::shared_ptr<AnimationEventData> data( new AnimationEventData );
            EventPtr event( new Event( finishEvent ) );

            data->animation = *i;
            event->data = data;

            queueEvent( event );

            if( (*i)->shouldRemoveWhenFinished() )
            {
                i = animations.erase( i );
                continue;
            }
        }

        i++;
    }
}

bool AnimationManager::frameStarted( const Ogre::FrameEvent& evt )
{
    timeSinceLastFrame = evt.timeSinceLastFrame;
    return true;
}