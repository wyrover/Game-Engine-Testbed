am = AnimationManager.getSingleton()
nv = NavigationMesh( Vector3.ZERO, Quaternion.IDENTITY, Vector3.UNIT_SCALE )
Ogre.destroyEntity'Nav'
ne = Ogre.createEntity( 'Nav', 'FloorNav.mesh' )
nv:buildFromEntity( ne )

function moveTo( v )
    local ma = MovementAnimation( player.node, v, 20 )
    am:add(ma)
    ma:start()
    return ma
end

function rotateTo( heading )
    local src = player.node:getOrientation() * Vector3.UNIT_X
    -- Prevent toppling over by restricting rotation to Y
    heading.y = src.y
    local quat = src:getRotationTo( heading, Vector3.UNIT_Y )
    local ra = RotationAnimation( player.node, quat, 5 )
    am:add(ra)
    ra:start()
    return ra
end

function getDirectionTo( v )
    local d = v - player.node:getPosition()
    d:normalise()
    return d
end

function followList( list )
    local wa = MeshAnimation( player.mesh, 'Walk' )
    am:add(wa)
    wa:setWeight(0)
    wa:setFadeSpeed(2)
    wa:start()
    wa:fadeIn()
    for i, vector in pairs(list) do
        print( 'Turning to face ', vector )
        local r = rotateTo( getDirectionTo( vector ) )
--        while not r:isFinished() do
--            yield()
--        end
        print( 'Heading off to ', vector )
        local m = moveTo( vector )
        while not m:isFinished() do
            yield()
        end
    end
    wa:fadeOut()
    while wa:isFadingOut() do
        yield()
    end
    am:remove(wa)
end

function getpath()
    local p = player.node:getPosition()
    local d = base:hitPosition(_X,_Y)
    local maxAngle = Radian( Degree(90) )
    
    return nv:findPath( p, d, maxAngle, 5 )
end

function walkTask()
    followList(path)
end

function walk()
    path = getpath()
    createTask( walkTask )
end