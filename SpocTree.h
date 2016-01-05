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
		
			// Depth is 1 or greater. We will continue to add 
			// children until we hit the max depth.
			// Max number of nodes we create + root.
			const int maxnodes = pow( 8, _depth ) + 1;
			// The number of nodes we've created so far.
			// About to add root.
			int numnodes = 1;
			// Queue for unprocessed buckets.
			// Put head of bucket list into queue.
			std::list <Spocket> tempqueue;
			tempqueue.push_back( bucketlist[0] );
			// This is used to track parent nodes for 
			// children.
			Spocket *sp = 0;

			// If there are nodes in the queue, continue to process 
			// them. Queue will get smaller as nodes are 
			// moved from the queue to our bucket list.
			while( tempqueue.size() ) {

				// 
				bucketlist.push_back( tempqueue.front() );
				tempqueue.pop_front();
				sp = &bucketlist.back();

				// 
				for( int c = 0; c < 8 && numnodes < maxnodes; c++ ) {
					// Child node length.
					vec3 clen = ((sp->poslm - sp->neglm) / 2);
					// Parent origin. Later calculated to 
					// child origin.
					vec3 opos = sp->neglm + clen;
					// Axis lengths.
					vec3 xvec = clen * vec3( 0.5f, 0, 0 );
					vec3 yvec = clen * vec3( 0, 0.5f, 0 );
					vec3 zvec = clen * vec3( 0, 0, 0.5f );
					// Adjust child origin.
					// Need a different(but particular) one for all eight.
					// Top childs.
					if( c == 0 ) opos = opos - xvec + yvec + zvec;
					if( c == 1 ) opos = opos + xvec + yvec + zvec;
					if( c == 2 ) opos = opos + xvec + yvec - zvec;
					if( c == 3 ) opos = opos - xvec + yvec - zvec;
					// Bottom childs.
					if( c == 4 ) opos = opos - xvec - yvec + zvec;
					if( c == 5 ) opos = opos + xvec - yvec + zvec;
					if( c == 6 ) opos = opos + xvec - yvec - zvec;
					if( c == 7 ) opos = opos - xvec - yvec - zvec;
					tempqueue.push_back( Spocket() );
					tempqueue.back().sparent = sp;
					tempqueue.back().poslm = opos + xvec + yvec + zvec;
					tempqueue.back().neglm = opos - xvec - yvec - zvec;
					numnodes++;
				}
			}

		} // buildtree()
};
