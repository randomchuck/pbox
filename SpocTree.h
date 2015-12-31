///////////////////////////////////////////////////////////////////////////////
// 
// Oct Tree + Sphere.
// 
// Creates an octree for collision detection.
// Accepts position and radius of sphere, but internally creates 
// "bounding boxes."

// Lists of things.
#include <vector>
// vectors and such.
#include "Glm_Lite.h"

///////////////////////////////////////////////////////////////////////////////
// Representation of sphere.
// position
// radius
// poslm and neglm are positive and negative limits.
// We calc them by position.x/y/z +/- radius. Should help us 
// avoid doing +/- when checking for collisions.
struct SFear {
	vec3 pos;
	float rad;
	vec3 poslm;
	vec3 neglm;
};

///////////////////////////////////////////////////////////////////////////////
// A SpocTree Bucket.
// Has 8 Spocket children or can be a leaf node.
// Spockets will also contain a list of 
// indices to SFear's.
// Similar to SFears, it has an upper and lower 
// limit in world coordinates.
struct Spocket {
	// Upper and lower limits to volume.
	vec3 poslm;
	vec3 neglm;
	// Children of this object.
	// If looking down at the volume(-Y), children order is clockwise:
	// childs[0] = UpperLeft/HighY, childs[1] = UpperRight/HighY, 
	// childs[2] = LowerLeft/HighY, childs[3] = LowerRight/HighY,
	// childs[4] = UpperLeft/LowY, childs[5] = UpperRight/LowY, 
	// childs[6] = LowerLeft/LowY, childs[7] = LowerRight/LowY
	Spocket *childs[8];
};

///////////////////////////////////////////////////////////////////////////////
// Sphere/Octree.
// 
// Feed it a list of spheres and it generates an octree for you.
// If you match the sphere list with your objects array, you can 
// just use indices to find specific spheres.
// 
class SpocTree {
	public:

		std::vector <SFear> slist;

		SpocTree() {}
		~SpocTree() {}
		
		void addsphere( const vec3 &_pos, float _radius ) {
			SFear nsfw;
			nsfw.pos = _pos;
			nsfw.rad = _radius;
			vec3 poslm = nsfw.pos + vec3( _radius, _radius, _radius );
			vec3 neglm = nsfw.pos - vec3( _radius, _radius, _radius );
			slist.push_back( nsfw );
		}

		void buildtree( void ) {

		}
};
