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
#ifndef SCRIPTINGSYSTEM_H_INCLUDED
#define SCRIPTINGSYSTEM_H_INCLUDED

#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <eventmanager.h>
#include <OgreFrameListener.h>

class ScriptingSystem : public EventListenerSender, public Ogre::FrameListener
{
    lua_State *mL;
    static int GUID;

public:
    friend void queueEventThunk( lua_State *, EventPtr );

    ScriptingSystem() : mL(0)
    {
    }
    ~ScriptingSystem()
    {
        shutdown();
    }

    void shutdown();
    void initialise();
    void bind();

    bool EventNotification( EventPtr event );

    bool frameStarted(const Ogre::FrameEvent& evt);
    bool frameEnded(const Ogre::FrameEvent& evt);

    lua_State* getInterpreter() { return mL; }
};

#endif // SCRIPTINGSYSTEM_H_INCLUDED