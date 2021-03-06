#! /usr/bin/env lua

--[[

packatlas.lua
-------------

Uses ImageMagik command line utilities to compose an atlas image 
and datafile for Gorilla.

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

]]--

images={}
placed={}

function getSize( filename )
    local p = io.popen( 'identify -format \'%w %h\' ' .. filename )
    local ret = p:read('*a')
    p:close()
    local split = string.find( ret, ' ' )
    return string.sub( ret, 1, split-1 ) + 0, string.sub( ret, split+1 ) + 0
end

function getFiles( listfile )
    for file in io.lines( listfile ) do
        local image = { x = 0, y = 0, image = file }
        image.w, image.h = getSize( file )
        table.insert( images, image )
    end
end

function imageArea( image )
    return image.w * image.h
end

function compare( a, b )
    return a.h > b.h
    --return imageArea(a) > imageArea(b)
end

function placeImage( index, x, y )
    local image = images[index]

    image.x = x
    image.y = y

    table.insert( placed, image )
    table.remove( images, index )
end

function fits( canvas, image )
    --print( 'Check fit:', image.w, image.h, canvas.w, canvas.h )
    if image.w <= canvas.w and image.h <= canvas.h then
        --print 'Yes'
        return true
    end
    --print 'No'
    return false
end

function showlists()
    print()
    print 'Remaining List'
    table.foreach( images, function( i, v ) print( v.x, v.y, v.w, v.h, v.image ) end ) 
    print()
    print 'Placed List'
    table.foreach( placed, function( i, v ) print( v.x, v.y, v.w, v.h, v.image ) end ) 
    print()
end

function fillcanvas( canvas )
    -- Find the bigest image that will fit and place it.
    -- Divide leftover space and call recursively.
    --
    if #images == 0 then return end -- no images left to fit
    if canvas.w <= 0 then return end
    if canvas.h <= 0 then return end

    --print( 'Canvas ', canvas.x, canvas.y, canvas.w, canvas.h )
    for i, v in ipairs( images ) do
        if fits( canvas, v ) then
            --print( 'Placing image', v.image, 'at', canvas.x, canvas.y )
            placeImage( i, canvas.x, canvas.y )
            -- Horizontal
            local subcanvas = { x = canvas.x + v.w, y = canvas.y, w = canvas.w - v.w, h = v.h } 
            fillcanvas( subcanvas )
            -- Vertical
            local subcanvas = { x = canvas.x, y = canvas.y + v.h, w = canvas.w, h = canvas.h - v.h }
            fillcanvas( subcanvas )

            return
        end
    end
end

function getImagePosition( imageName )
    -- Find the whitepixel image, and offset into it.
    local image

    table.foreach( placed, function(k,v) if v.image == imageName .. '.png' then image = v end end )

    if image == nil then error( 'Could not find ' .. imageName .. ' image!') end

    return image.x, image.y 
end

function compositePlacedImage( image, gorillaFile )

    print( 'Compositing image from ' .. image.image )

    local cmd, desc
    cmd = 'composite -geometry +' .. image.x .. '+' .. image.y ..' ' .. image.image
    cmd = cmd .. ' ' .. compositeFile .. '.png ' .. compositeFile .. '.png'
    os.execute( cmd )
    
    desc = string.sub( image.image, 1, string.find( image.image, '.png' )-1 )
    desc = desc .. ' ' .. image.x .. ' ' .. image.y
    desc = desc .. ' ' .. image.w .. ' ' .. image.h
    gorillaFile:write( desc .. '\n' )
end

function processPlacedImages()
    os.execute( 'cp ' .. canvas.image .. ' ' .. compositeFile .. '.png' )
    local gorillaFile = io.open( compositeFile .. '.gorilla', 'w+' )

    -- Write info from canvas's gorilla file (font stuff)
    local canvasGorilla = io.open( canvas.gorilla )
    gorillaFile:write( canvasGorilla:read('*a') )
    canvasGorilla:close()

    gorillaFile:write( '[Sprites]\n' )

    table.foreach( placed, function(k, v) compositePlacedImage( v, gorillaFile ) end )
    
    gorillaFile:write( '[Texture]\n' )
    gorillaFile:write( 'file ' .. compositeFile .. '.png\n' )
    local x, y = getImagePosition( 'white' )
    gorillaFile:write( 'whitepixel ' .. x+2 .. ' ' .. y+2 .. '\n' )
    gorillaFile:write( '\n' )
    
    -- Read in font data and shift co-ordinates to where the image was placed.
    table.foreach( fonts, function( k, v ) processFont( v, gorillaFile ) end )

    gorillaFile:close()
end

function processFont( fontName, gorillaFile )
    print( 'Loading font: ' .. fontName .. '.lua' )
    dofile( fontName .. '.lua' )    -- load font data.
    local offsetx, offsety = getImagePosition( fontName )
    gorillaFile:write( '[Font.' .. font.size .. ']\n' )
    gorillaFile:write( 'lineheight ' .. font.lineheight .. '\n' )
    gorillaFile:write( 'spacelength ' .. font.spacelength .. '\n' )
    gorillaFile:write( 'baseline ' .. font.baseline .. '\n' )
    gorillaFile:write( 'letterspacing ' .. font.letterspacing .. '\n' )
    gorillaFile:write( 'monowidth ' .. font.monowidth .. '\n' )
    gorillaFile:write( 'range ' .. font.range.start .. ' ' .. font.range.finish .. '\n' )
    gorillaFile:write( 'offset ' .. offsetx .. ' ' .. offsety .. '\n' )
    for k,v in pairs( font.glyphs ) do
        gorillaFile:write( 'glyph_' )
        gorillaFile:write( k .. ' ' )
        gorillaFile:write( v[1] .. ' ' )
        gorillaFile:write( v[2] .. ' ' )
        gorillaFile:write( v[3] .. ' ' )
        gorillaFile:write( v[4] .. ' ' )
        gorillaFile:write( v[5] .. '\n' )
    end
    for k,v in pairs( font.kerning ) do
        gorillaFile:write( 'kerning_' .. k .. ' ' )
        gorillaFile:write( v.left .. ' ' )
        gorillaFile:write( v.k .. '\n' )
    end
    for k,v in pairs( font.verticalOffset ) do
        gorillaFile:write( 'verticalOffset_' .. k .. ' ' .. v .. '\n' )
    end

end

canvas = { x=0, y=0, image = 'canvas.png', gorilla='canvas.gorilla' }
canvas.w, canvas.h = getSize( canvas.image )
compositeFile = 'atlas'
fonts = { 
    'acoustic_bass_regular_10',
    'dejavu_sans_book_14',
    'acoustic_bass_regular_20'
}

getFiles( 'imagelist.txt' )

table.sort( images, compare )

fillcanvas( canvas )
showlists()

if #images ~= 0 then
    print( 'Failed to fit all images.' )
    processPlacedImages()
else
    print( 'Fited all images within a canvas of ' .. canvas.w ..'x'..canvas.h )
    processPlacedImages()
end
