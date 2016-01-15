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
	
	// Initializes Spocket.
	Spocket() {
		poslm = vec3( 0, 0, 0 );
		neglm = poslm;
		sindices.clear();
		for( int c = 0; c < 8; c++ )
			childs[c] = 0;
	}
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
		std::list <Spocket> bucketlist;

		// The number of nodes the tree has.
		int numnodes;

		///////////////////////////////////////////////////////////////////////
		// Def C-Tor.
		SpocTree(): numnodes(0) {  }
		///////////////////////////////////////////////////////////////////////
		// Def Destructor.
		~SpocTree() { clear(); }
	
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

			// We're rebuilding the tree so no nodes yet.
			numnodes = 0;

			// Clear the bucket list.
			bucketlist.clear();

			// Create new root.
			Spocket sproot;

			// Set bounds of root.
			sproot.poslm = vec3( _size ) + _pos;
			sproot.neglm = sproot.poslm * -1 + _pos;

			// If depth is 0, add indices to root and return.
			if( _depth == 0 ) {
				for( unsigned int s = 0; s < slist.size(); s++ )
					sproot.sindices.push_back( s );
				bucketlist.push_back( sproot );
				numnodes = 1;
				return;
			}
		
			// Depth is 1 or greater. We will continue to add 
			// children until we hit the max depth.
			// Max number of nodes we create + root.
			const int maxnodes = pow( 8, _depth ) + 1;
			// The number of nodes we've created so far.
			// About to add root.
			numnodes = 1;
			// This is used to track parent nodes for 
			// children.
			Spocket *sp = 0;
			// Add root to list.
			bucketlist.push_back( sproot );

			// Iterator for bucket list. Ugh...
			std::list<Spocket>::iterator buckit = bucketlist.begin();

			// Loop until we've created enough nodes.
			while( numnodes < maxnodes ) {
				
				// Point to current parent.
				sp = &*buckit;

				// Create 8 child nodes.
				for( int c = 0; c < 8; c++ ) {
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
					bucketlist.push_back( Spocket() );
					bucketlist.back().poslm = opos + xvec + yvec + zvec;
					bucketlist.back().neglm = opos - xvec - yvec - zvec;
					sp->childs[c] = &bucketlist.back();
					numnodes++;
				}
				buckit++;

			} // while()

			// Put spheres in their place...
			addspherestotree();

		} // buildtree()
		
		///////////////////////////////////////////////////////////////////////
		// Takes a vector3 which contains a sphere's position and radius.
		// vec3( p.x, p.y, p.z, radius ).
		// Also accepts box bounds -> 
		// _box[0] = positive point. _box[1] = negative point.
		// 
		// Now it doesn't work quite like you would think. It creates a 
		// bounding box for the sphere using the pos and radius. It then 
		// checks every point from that gen'd box and tests if contained 
		// within _box. If any of those points are within _box, this returns 
		// true.
		// 
		// So considering that, it's possible for a sphere to be too big and 
		// be missed by certain _box's. Just make sure _box's are always bigger 
		// than the spheres and this won't happen.
		bool sphereboxinbox( const vec3 &s, const vec3 _box[2] ) {
		
			// Pull out pos and radius.
			vec3 ps = s;
			float rd = s.w;	

			// Front Face.
			// Low x, Low y, Low z(close to cam)
			if( pntinbox( ps + vec3(-rd, -rd, -rd), _box ) )
				return true;
			// Low x, High y, Low z.
			if( pntinbox( ps + vec3(-rd, rd, -rd), _box ) )
				return true;
			// High x, High y, Low z.
			if( pntinbox( ps + vec3(rd, rd, -rd), _box ) )
				return true;
			// High x, Low y, Low z.
			if( pntinbox( ps + vec3(rd, -rd, -rd), _box ) )
				return true;
			// Back Face.
			// Low x, Low y, High z.
			if( pntinbox( ps + vec3(-rd, -rd, rd), _box ) )
				return true;
			// Low x, High y, High z.
			if( pntinbox( ps + vec3(-rd, rd, rd), _box ) )
				return true;
			// High x, High y, High z.
			if( pntinbox( ps + vec3(rd, rd, rd), _box ) )
				return true;
			// High x, Low y, High z.
			if( pntinbox( ps + vec3(rd, -rd, rd), _box ) )
				return true;
			// If we made it here, then none of the sphere 
			// points are contained within the box.
			return false;

		} // sphereboxinbox()

		///////////////////////////////////////////////////////////////////////
		// Pass a vec3 and a vec3[2]. Expects _box[0] to be poslm - 
		// ie, positive limit of box, and _box[1] to be negative.
		// Returns true if _pnt is within this box.
		bool pntinbox( const vec3 &_pnt, const vec3 _box[2] ) {
			
			// X
			if( _pnt.x > _box[0].x || _pnt.x < _box[1].x )
				return false;
			// Y
			if( _pnt.y > _box[0].y || _pnt.y < _box[1].y ) 
				return false;
			// Z
			if( _pnt.z > _box[0].z || _pnt.z < _box[1].z )
				return false;
			// The point is in the box.
			return true;

		} // pntinbox()


		///////////////////////////////////////////////////////////////////////
		// Add spheres/indices to appropriate bucket/leaf nodes in tree.
		// 
		// Note: Very slow. Need to optimize so we're not potentially 
		// checking every child. Follow the hierarchy.
		void addspherestotree( void ) {
			// Iterator for bucket list. Ugh...
			std::list<Spocket>::iterator buckit = bucketlist.begin();

			// All spheres, add to tree leaf nodes.
			for( unsigned int sidx = 0; sidx < slist.size(); sidx++ ) {
				// Grab sphere position and radius.
				vec3 spheer = vec3( slist[sidx].pos );
				spheer.w = slist[sidx].rad;
				// Search bucketlist for nodes that could contain 
				// this sphere.
				for( buckit = bucketlist.begin(); buckit != bucketlist.end(); buckit++ ) {
					// Only do checks on leaf nodes.
					if( (*buckit).childs[0] ) continue;
					// Build second box. Sphere might be in it.
					vec3 bx[2];
					bx[0] = (*buckit).poslm;
					bx[1] = (*buckit).neglm;
					// Finally check to see if any part of the sphere is 
					// within the box.
					if( sphereboxinbox(spheer, bx) )
						(*buckit).sindices.push_back( sidx );

				} // for( buckit =...

			} // for( unsigned int sidx...
		}

		///////////////////////////////////////////////////////////////////////
		// Cleans up lists/memory.
		void clear( void ) {
			// Empty all of the lists.
			slist.clear();
			bucketlist.clear();
			numnodes = 0;
		}
};
