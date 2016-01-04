///////////////////////////////////////////////////////////////////////////////
// 
// Oct Tree + Sphere.
// 
// Creates an octree for collision detection.
// Accepts position and radius of sphere, but internally creates 
// "bounding boxes."

// Lists of things.
#include <vector>
#include <list>
// vectors and such.
#include "Glm_Lite.h"

///////////////////////////////////////////////////////////////////////////////
// Representation of sphere.
// position
// radius
// poslm and neglm are positive and negative limits.
// We calc them by position.x/y/z +/- radius. Should help us 
// avoid doing +/- when checking for collisions.
struct Sfear {
	vec3 pos;
	float rad;
	vec3 poslm;
	vec3 neglm;
};

///////////////////////////////////////////////////////////////////////////////
// A SpocTree Bucket.
// Has 8 Spocket children or can be a leaf node.
// Spockets will also contain a list of 
// indices to Sfear's.
// Similar to Sfears, it has an upper and lower 
// limit in world coordinates.
struct Spocket {
	// One node up in the hierarchy.
	Spocket *sparent;
	// Upper and lower limits to volume.
	vec3 poslm;
	vec3 neglm;
	// Indices into sphere list.
	std::vector <int> sindices;
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

		// Sfear(Sphere) list.
		std::vector <Sfear> slist;

		// List of buckets. Root is the first element.
		std::vector <Spocket> bucketlist;

		///////////////////////////////////////////////////////////////////////
		// Def C-Tor.
		SpocTree() {  }
		///////////////////////////////////////////////////////////////////////
		// Def Destructor.
		~SpocTree() {  }
	
		///////////////////////////////////////////////////////////////////////
		// Add a sphere to the list.
		// _pos - position.
		// _radius...
		void addsphere( const vec3 &_pos, float _radius ) {
			Sfear nsfw;
			nsfw.pos = _pos;
			nsfw.rad = _radius;
			vec3 poslm = nsfw.pos + vec3( _radius, _radius, _radius );
			vec3 neglm = nsfw.pos - vec3( _radius, _radius, _radius );
			slist.push_back( nsfw );
		}

		///////////////////////////////////////////////////////////////////////
		// After addsphere()-ing a bunch of spheres, call buildtree()
		// to create the octree. You should only need to call this once during 
		// initialization.
		// 
		// _depth determines how many times we split up the tree.
		// Can improve/degrade performance.
		// A depth of 1 is default and will produce the root and 8 children.
		// Depth of 0 will just have the root, which is just a big bounding 
		// box containing all Sfears.
		// 
		// _size is the size of the octree volume. values must be positive.
		// Can improve/degrade performance.
		// 
		// _pos is the position/offset of the tree in world coordinates.
		// You can use it to offset the nodes so Sfears register in fewer 
		// buckets.
		// Helps scenarios where objects are axis aligned.
		// Can improve/degrade performance.
		void buildtree( const int _depth = 1, 
						const vec3 &_size = vec3(100, 100, 100),  
						const vec3 &_pos = vec3(0, 0, 0) ) {
			// Make sure user didn't pass something less than 0.
			if( _depth < 0 ) return;

			// Clear the bucket list.
			bucketlist.clear();

			// Create new root.
			bucketlist.push_back( Spocket() );

			// Set bounds of root.
			bucketlist[0].poslm = vec3( _size ) + _pos;
			bucketlist[0].neglm = bucketlist[0].poslm * -1 + _pos;

			// If depth is 0, add indices to root and return.
			if( _depth == 0 ) {
				for( int s = 0; s < slist.size(); s++ )
					bucketlist[0].sindices.push_back( s );
				return;
			}
		
					//spkparent = (i == 0) ? templist[templist.size() - 1] : spkparent;
					//bucketlist.push_back( Spocket() );
					//const int bksize = bucketlist.size() - 1;
					//bucketlist[ bksize ].poslm = vec3( spkparent->poslm / 2 );
					//bucketlist[ bksize ].neglm = vec3( spkparent->neglm / 2 );
					//bucketlist[ bksize ].sindices.push_back( 0 );
					//bucketlist[ bksize ].childs[0] = 0;
			// Depth is 1 or greater. We will continue to add 
			// children until we hit the max depth.
			const int maxnodes = pow( 8, _depth );
			int curnodes = 1;
			std::list <Spocket> tempqueue;
			tempqueue.push_back( bucketlist[0] );
			Spocket *spkparent = 0;
			while( curNodes >= maxnodes ) {
				if( curnodes > 1 ) {
					bucketlist.push_back( tempqueue.front() );
					tempqueue.pop_front();

				}
				tempqueue.push_back( Spocket() );
				tempqueue[ tempqueue.size() - 1 ].sparent = 
				tempqueue[ tempqueue.size() - 1 ].poslm = vec3(  );
				tempqueue[ tempqueue.size() - 1 ].neglm = vec3(  );

				curNodes += 8;
			}

		} // buildtree()
};
