///////////////////////////////////////////////////////////////////////////////
// 
// PCollision - Physics Collision(for PBox)
// 
// Contains data about a collision between two PBox's...
// * XYZ for point of collision.
// * Face and normal info for collision point.
// 
///////////////////////////////////////////////////////////////////////////////

// 4x4 Mat's and Vec3's.
#include "GLM_Lite.h"

///////////////////////////////////////////////////////////////////////////////
// Point of collision between two boxes.
class PCPoint {
	public:
		// 0 for box 1, 1 for box 2.
		int boxid;
		// Point of collision.
		vec3 pnt;
		// The face the point was detected on.
		vec3 face[4];
		// That face's index.
		int faceidx;
		// Face normal.
		vec3 fnormal;
		// Dummy default constructor.
		PCPoint() {}
		// Parameterized Constructor.
		// Can add all data needed to describe collision point.
		PCPoint( const int _boxid, int _faceidx, const vec3 &_point, const vec3 _face[4], const vec3 _facenormal ) {
			boxid = _boxid;
			faceidx = _faceidx;
			pnt = _point;
			for( int f = 0; f < 4; f++ )
				face[f] = _face[f];
			fnormal = _facenormal;
		}
};

///////////////////////////////////////////////////////////////////////////////
// Collision Information between two PBoxes.
class PCollision {
	public:
		// Number of collision points.
		int numcolpnts;
		// The collision points.
		PCPoint colpnts[50];
		// Def C-tor.
		PCollision(): numcolpnts(0) {}
		// Adds point to the list.
		void addpoint( int _boxid, int _faceidx, const vec3 &_pnt, const vec3 _face[4], const vec3 _facenormal ) {
			colpnts[ numcolpnts++ ] = PCPoint( _boxid, _faceidx, _pnt, _face, _facenormal );
		}
		// Calcs the average collision position.
		vec3 averagepoint( void ) {
			if( numcolpnts == 0 )
				return vec3( -1000.0f, -1000.0f, -1000.0f );
			vec3 avgpnt;
			for( int p = 0; p < numcolpnts; p++ )
				avgpnt = avgpnt + colpnts[p].pnt;
			return avgpnt / numcolpnts;
		}
		// Calcs average normals for box1 and box2.
		void averagenormals( vec3 &_box1norm, vec3 &_box2norm ) {
			// Get the average normal from all faces involved.
			for( int fn = 0; fn < numcolpnts; fn++ )
				if( colpnts[fn].boxid == 0 )
					_box1norm = _box1norm + colpnts[fn].fnormal;
				else
					_box2norm = _box2norm + colpnts[fn].fnormal;
		}
		// Same as averagenormals(), but excludes all but one face.
		// Will average normals from multiple faces anyway if 
		// there are an equal number of contact points on at least 
		// two faces.
		// Using this instead of averagenormals() helps mitigate a 
		// problem with a box resting on top of another, face to face.
		// Since we would get collisions on side-faces too, the 
		// averaged normal included a "sideways" vector, which would 
		// wiggle the top box until it fell off.
		void averagenormals1f( vec3 &_box1norm, vec3 &_box2norm ) {
			// Count contact points for each face.
			int highcntb1[6] = {0};
			int highcntb2[6] = {0};
			int b1high = 0;
			int b2high = 0;
			int prevb1high = 0;
			int prevb2high = 0;
			int b1cpntidx = 0;
			int b2cpntidx = 0;
			// Loop through every contact point and count for 
			// every face.
			for( int f = 0; f < numcolpnts; f++ ) {
				// First box.
				if( colpnts[f].boxid == 0 ) {
					// Go ahead and average the normal.
					_box1norm = _box1norm + colpnts[f].fnormal;
					// Add a count to this face.
					highcntb1[ colpnts[f].faceidx ]++;
					// The face we just counted has a higher number of hits 
					// than the previous high, record it.
					if( highcntb1[ colpnts[f].faceidx ] > highcntb1[b1high] ) {
						// Store previous high and current.
						prevb1high = b1high;
						b1high = colpnts[f].faceidx;
						b1cpntidx = f;
					}
				}
				else { // Second box.
					// Average the normal.
					_box2norm = _box2norm + colpnts[f].fnormal;
					// Count face.
					highcntb2[ colpnts[f].faceidx ]++;
					// Is it higher than the last?
					if( highcntb2[ colpnts[f].faceidx ] > highcntb2[b2high] ) {
						// Store previous high and current.
						prevb2high = b2high;
						b2high = colpnts[f].faceidx;
						b2cpntidx = f;
					}
				}
			}
			// If we have a "winner" then just return that face's normal.
			// If there is a tie, we'll just let the averaged normal pass 
			// through.
			if( highcntb1[b1high] > highcntb1[prevb1high] )
				_box1norm = colpnts[b1cpntidx].fnormal;
			// 
			if( highcntb2[b2high] > highcntb2[prevb2high] )
				_box2norm = colpnts[b2cpntidx].fnormal;
			
		}

};
