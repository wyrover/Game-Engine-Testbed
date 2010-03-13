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

#include <navigationmesh.h>
#include <algorithm>

NavigationCell::NavigationCell( Ogre::Vector3 a, Ogre::Vector3 b, Ogre::Vector3 c )
{
    mVertices[0] = a;
    mVertices[1] = b;
    mVertices[2] = c;
    mLinks[0] = mLinks[1] = mLinks[2] = 0;
    mCentre = ( a + b + c ) / 3;
}

bool NavigationCell::hasVertex( Ogre::Vector3& vec )
{
    if( mVertices[0] == vec )   return true;
    if( mVertices[1] == vec )   return true;
    if( mVertices[2] == vec )   return true;
    return false;
}

NavigationMesh::NavigationMesh( Ogre::Vector3 position /* = Ogre::Vector3::ZERO */ ,
                                Ogre::Quaternion rotation /* = Ogre::Quaternion::IDENTITY */ ,
                                Ogre::Vector3 scale /* = Ogre::Vector3::UNIT_SCALE */ ) :
    mPosition( position ),
    mRotation( rotation ),
    mScale( scale )
{
}

NavigationMesh::~NavigationMesh()
{
}

void NavigationMesh::BuildFromOgreMesh( Ogre::MeshPtr mesh )
{
    using namespace Ogre;

    size_t vertex_count;
    Vector3* vertices;
    size_t index_count;
    unsigned long* indices;

    OgreTools::GetMeshInformation( mesh, vertex_count, vertices, index_count, indices,
                                   mPosition, mRotation, mScale );

    // Add each triangle as a navigation cell.
    for( size_t i = 0; i < index_count; i += 3 )
    {
        NavigationCell cell( vertices[indices[i]], vertices[indices[i+1]], vertices[indices[i+2]] );

        mCells.push_back( cell );
    }

    delete [] vertices;
    delete [] indices;

    // Compute neighbours.
    // There will be a better way, but this works for now
    // and this does not run very often.

    CellVector::iterator current;
    CellVector::iterator test;

    for( current = mCells.begin(); current != mCells.end(); current++ )
    {
        for( test = mCells.begin(); test != mCells.end(); test++ )
        {
            if( current != test )
            {
                if( test->hasVertex( current->mVertices[0] ) )
                {
                    if( test->hasVertex( current->mVertices[1] ) )
                    {
                        current->mLinks[0] = &(*test);
                        continue;
                    }
                    if( test->hasVertex( current->mVertices[2] ) )
                    {
                        current->mLinks[2] = &(*test);
                        continue;
                    }
                }
                if( test->hasVertex( current->mVertices[1] ) && test->hasVertex( current->mVertices[2] ) )
                {
                    current->mLinks[1] = &(*test);
                }
            }
        }
    }

    return;
}

// This assumes a nearly flat navigation mesh. I.e. there are no triangle normals more than 90deg
// away from the Y axis (UP/DOWN).
NavigationCell* NavigationMesh::getCellContainingPoint( Ogre::Vector3& p )
{
    using namespace Ogre;

    Vector2 a;
    Vector2 b;
    Vector2 c;
    Vector2 p2D( p.x, p.z );

    for( CellVector::iterator i = mCells.begin(); i != mCells.end(); i++ )
    {
        // Chuck away Y component
        a.x = i->mVertices[0].x;        a.y = i->mVertices[0].z;
        b.x = i->mVertices[1].x;        b.y = i->mVertices[1].z;
        c.x = i->mVertices[2].x;        c.y = i->mVertices[2].z;

        if( Math::pointInTri2D( p2D, a, b, c ) )
            return &(*i);
    }

    return 0;
}

NavigationPath* NavigationMesh::findNavigationPath( Ogre::Vector3 position, Ogre::Vector3 destination )
{
    NavigationCell* destinationCell = getCellContainingPoint( destination );
    if( destinationCell == 0 )
        return 0;   // Destination is not within the navigation mesh.

    NavigationCell* currentCell = getCellContainingPoint( position );
    if( currentCell == 0 )
        return 0;   // Current position is not within the navigation mesh.

    NavigationCellList* cellPath = findNavigationCellPath( currentCell, destinationCell );

    if( cellPath == 0 )
        return 0;   // No posible path to destination.

    // Create point path from returned cell path.
    NavigationPath* path = new NavigationPath;

    path->push_back( position );   // Will be needed for path smoothing.

    for( NavigationCellList::iterator i = cellPath->begin(); i != cellPath->end(); i++ )
    {
        // Use the mid point of the cell side we should use to leave this cell.
        switch( (*i)->path )
        {
            case 0: // Leave via side 0
                path->push_back( (*i)->mVertices[0].midPoint( (*i)->mVertices[1] ) );
                break;
            case 1: // Leave via side 1
                path->push_back( (*i)->mVertices[1].midPoint( (*i)->mVertices[2] ) );
                break;
            case 2: // Leave via side 2
                path->push_back( (*i)->mVertices[2].midPoint( (*i)->mVertices[0] ) );
                break;
            default:
                // What? the mPath should be set if the cell is in the list....
                assert(0);
        }
    }

    path->push_back( destination );

    delete cellPath;

    // Smooth out path? Catmull Rom thingy goes here... ( or called seperately on path ).

    return path;
}

NavigationCellList* NavigationMesh::findNavigationCellPath( NavigationCell* position, NavigationCell* destination )
{
    NavigationCell* currentCell;
    bool pathFound = false;

    resetPathfinding();

    // Run the A* algorithm from destination to current position.
    pushIntoOpenList( destination );

    destination->g_cost = 0;
    destination->parent = 0;

    while( ! mOpenList.empty() )
    {
        currentCell = popFromOpenList();

        if( currentCell == position )
        {
            // Yahoo, we found our way.
            pathFound = true;
            break;
        }

        // This cell has now been visted.
        currentCell->isClosed = true;

        // For each neighbour of the current cell.
        for( int linkIndex = 0; linkIndex < 3; linkIndex++ )
        {
            NavigationCell* neighbour = currentCell->mLinks[linkIndex];

            if( ! neighbour )   // No neighbor in that direction
                continue;

            if( currentCell->parent == neighbour )  // Kinda pointless
                continue;

            Ogre::Real newcost = currentCell->g_cost + neighbour->mCentre.distance( currentCell->mCentre );

            if( neighbour->isClosed == false && neighbour->isOpen == false )
            {
                // This is the first time we have been to this cell.
                neighbour->g_cost = newcost;
                neighbour->h_cost = aStarHeuristic( neighbour, position );
                neighbour->totalcost = neighbour->g_cost + neighbour->h_cost;
                neighbour->parent = currentCell;
                pushIntoOpenList(neighbour);
                continue;
            }

            // We've been here before... see if we have a better path now.
            if( newcost < neighbour->g_cost )
            {
                // We have a better path to this cell (neighbour).  Recompute data and
                // either re-add or promote in the open list.
                neighbour->g_cost = newcost;
                // h cost does not change.
                neighbour->totalcost + neighbour->g_cost + neighbour->h_cost;
                neighbour->parent = currentCell;

                // possibly undo closed status
                neighbour->isClosed = false;

                if( neighbour->isOpen == false )
                    pushIntoOpenList( neighbour );
                else
                    promoteCellInOpenList( neighbour );
            }
        }
    }

    if( ! pathFound )
        return 0;

    // Make list.
    // currentCell = position from above code.
    NavigationCellList* path = new NavigationCellList();

    while( currentCell != destination )
    {
        for( int linkIndex = 0; linkIndex < 3; linkIndex++ )
        {
            if( currentCell->mLinks[linkIndex] == currentCell->parent )
            {
                currentCell->path = linkIndex;
                break;
            }
        }
        path->push_back( currentCell );
        currentCell = currentCell->parent;
    }

    return path;
}

Ogre::Real NavigationMesh::aStarHeuristic( NavigationCell* cell, NavigationCell* destination )
{
    // Tweek here!
    return cell->mCentre.distance( destination->mCentre );
}

void NavigationMesh::resetPathfinding()
{
    mOpenList.clear();
    for( CellVector::iterator i = mCells.begin(); i != mCells.end(); i++ )
    {
        i->path = -1;   // Not strictly required.
        i->parent = 0;  // Or this either.

        i->isOpen = false;
        i->isClosed = false;
    }
}

void NavigationMesh::pushIntoOpenList( NavigationCell* cell )
{
    cell->isOpen = true;
    mOpenList.push_back( cell );
    std::push_heap( mOpenList.begin(), mOpenList.end(), NavigationCellComparison() );
}

NavigationCell* NavigationMesh::popFromOpenList()
{
    NavigationCell* ret = mOpenList.front();

    std::pop_heap( mOpenList.begin(), mOpenList.end(), NavigationCellComparison() );
    mOpenList.pop_back();

    ret->isOpen = false;

    return ret;
}

void NavigationMesh::promoteCellInOpenList( NavigationCell* cell )
{
    for( NavigationCellList::iterator i = mOpenList.begin(); i != mOpenList.end(); i++ )
    {
        if( *i == cell )
        {
            // Reorder heap below and including this cell, to move it up the list.
            std::push_heap( mOpenList.begin(), i + 1, NavigationCellComparison() );
            return;
        }
    }

    // We only get here if the cell was not in the open list.
    assert( 0 );

    return;
}

void NavigationMesh::DebugTextDump( std::ostream &out )
{
    out << "Navigation Mesh" << std::endl;

    for( CellVector::iterator i = mCells.begin(); i != mCells.end(); i++ )
    {
        out << "Cell: " << &(*i) << std::endl;
        out << i->mVertices[0] << std::endl;
        out << i->mVertices[1] << std::endl;
        out << i->mVertices[2] << std::endl;
        out << "Link 1: " << i->mLinks[0] << std::endl;
        out << "Link 2: " << i->mLinks[1] << std::endl;
        out << "Link 3: " << i->mLinks[2] << std::endl;
        out << "Path: " << (i->path + 1) << std::endl;
    }
}