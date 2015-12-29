///////////////////////////////////////////////////////////////////////////////
// 
// PBox - Physics Box.
// 
// PBox is a simple, self contained, dynamic 3D cube.

// * Can be just a 3D object/box in space.
// - You specify the location/orientation/rotation.

// * Has collision detection with other PBoxes built into the class.
// - This works with a simple check first, then proper line to face checks.
// - Returns a list of points on the CALLING box that are hitting something.
// 
// * Dynamics/reactions/physics are also a feature.
// - Can apply velocities and accelerations.
// - Random forces.
// - Reactions to collisions.
// -- Utilizes collision points, rotation angles, and applies other forces.

// sqrt() and such.
#include <math.h>

// 4x4 Matrix and 3f Vector.
#include "GLM_Lite.h"

// Physics Collision.
// Holds info about our PBox collisions.
#include "PCollision.h"

// Useful for determining if certain functions passed/failed.
vec3 BADVECTOR( -1000.0f, -1000.0f, -1000.0f );

///////////////////////////////////////////////////////////////////////////////
// Physics Box.
class PBox {
	public:
		int winheight;
		// Position of box.
		vec3 pos;
		// Scale of box.
		vec3 scl;
		// Rotation axis.
		vec3 raxis;
		// Rotation angle around axis.
		float rangle;
		// Compiled matrix from pos/scl/raxis/rangle.
		mat4 mat;
		// Transformed points. pnts[0-8] * mat
		vec3 pnts[8];
		// Un-transformed points.
		// Same as pnts, except no * mat.
		vec3 pntsu[8];
		// How fast our box is moving.
		vec3 vel;
		// Rate at which the velocity changes.
		vec3 accel;
		// Specifies whether this box moves, or 
		// can be moved.
		bool dynamic;

		// These helper variables keep us from creating 
		// objects every frame. Improves performance.

		// Line in triangle result.
		vec3 lit_tris[2][3];
		// Collision info for this box.
		PCollision pc;
		// Store the largest dimension for this box.
		// We use it for sphere to sphere checks.
		// Improves performance.
		float largestaxis;

		// We apply this rotation to the box if it isn't colliding with 
		// anything. Mimics angular momentum.
		vec3 lastrotaxis;
		float lastrotangle;

		/////////////////////////////////////////////////////////////////////////////
		// Constructor - Parameterized.
		// Can give 0 or more parameters as needed.
		// Position, width/height/depth, scale, rotation axis, angle.
		PBox( vec3 _pos = vec3(0, 0, 0), vec3 _whd = vec3(1, 1, 1), vec3 _scale = vec3(1, 1, 1), vec3 _rot = vec3(0, 0, 0), float _angle = 0, bool _dynamic = true ) {
			// Front Face.
			pntsu[0] = vec3( -_whd.x / 2, -_whd.y / 2, -_whd.z / 2 );
			pntsu[1] = vec3( -_whd.x / 2,  _whd.y / 2, -_whd.z / 2 );
			pntsu[2] = vec3(  _whd.x / 2,  _whd.y / 2, -_whd.z / 2 );
			pntsu[3] = vec3(  _whd.x / 2, -_whd.y / 2, -_whd.z / 2 );
			// Back Face.
			pntsu[4] = vec3(  _whd.x / 2, -_whd.y / 2, _whd.z / 2 );
			pntsu[5] = vec3( -_whd.x / 2, -_whd.y / 2, _whd.z / 2 );
			pntsu[6] = vec3( -_whd.x / 2,  _whd.y / 2, _whd.z / 2 );
			pntsu[7] = vec3(  _whd.x / 2,  _whd.y / 2, _whd.z / 2 );
			// Copy untranslated points to translated points.
			// They aren't translated yet.
			pointsu( pnts );
			// ...now they are.
			settransform( _pos, _scale, _rot, _angle );
			// Most boxes are dynamic.
			dynamic = _dynamic;
			// Store widest/highest/deepest axis for sphere checks.
			largestaxis = calclargeaxis() * 2.0f;
			// No rotations from reactions have been applied yet.
			lastrotaxis = vec3( 0, 0, 0 );
			lastrotangle = 0.0f;
		}

		/////////////////////////////////////////////////////////////////////////////
		// Find largest axis of this box. Can only be done after everything has been scaled.
		float calclargeaxis( void ) {
			float lgax = -1;
			mat4 sclmtx = scale( scl );
			for( int ax = 0; ax < 8; ax++ ) {
				mat4 tpntmtx = sclmtx * mat4( pntsu[ax] );
				vec3 npnt;
				npnt.x = fabs( tpntmtx.columns[3].x );
				npnt.y = fabs( tpntmtx.columns[3].y );
				npnt.z = fabs( tpntmtx.columns[3].z );
				lgax = ( npnt.x > lgax ) ? npnt.x : lgax;
				lgax = ( npnt.y > lgax ) ? npnt.y : lgax;
				lgax = ( npnt.z > lgax ) ? npnt.z : lgax;
			}
			return lgax;
		}

		/////////////////////////////////////////////////////////////////////////////
		// Set whether the box moves, or can be moved.
		// True for yes, False for no.
		void setdynamic( bool _dynamic ) {
			dynamic = _dynamic;
		}
		/////////////////////////////////////////////////////////////////////////////
		// dynamic's getter.
		bool getdynamic( void) { return dynamic; }

		/////////////////////////////////////////////////////////////////////////////
		// Stores position/scale/rotation, rebuilds box transform, and 
		// applies it to the points. Use when you want to update position, 
		// scale, and rotation in one call.
		// 
		// A little note on const vec3 &. By putting const in front of our reference,
		// it allows us to do this -> setTransform(vec3(1, 1, 1)... Wouldn't be able to use
		// a temp vec3 without it.
		void settransform( const vec3 &_pos, const vec3 &_scale, const vec3 &_rot, float _angle ) {
			pos = vec3( _pos.x, _pos.y, _pos.z, 1 );
			scl = _scale;
			raxis = _rot;
			rangle = _angle;
			mat = buildtransform( _pos, _scale, _rot, _angle );
			transformpoints( pntsu, pnts, mat );
		}
		/////////////////////////////////////////////////////////////////////////////
		// Box's transform's getter.
		mat4 gettransform( void ) { return mat; }

		/////////////////////////////////////////////////////////////////////////////
		// If you need a transform matrix built but don't want to set the box's.
		// Returns a 4x4 matrix.
		mat4 buildtransform( const vec3 &_pos, const vec3 &_scale, const vec3 &_rot, float _angle ) {
			mat4 newMat;
			newMat = rotate( _angle, _rot );
			newMat *= scale( _scale );
			newMat.columns[3] = _pos;
			newMat.columns[3].w = 1.0f;
			return newMat;
		}

		/////////////////////////////////////////////////////////////////////////////
		// Set position/translation. Rebuilds box transform and applies to points.
		void setpos( const vec3 &_pos ) {
			pos = _pos;
			settransform( pos, scl, raxis, rangle );
		}
		/////////////////////////////////////////////////////////////////////////////
		// Getter for box position.
		// Opposite of setpos().
		vec3 getpos( void ) { return pos; }

		/////////////////////////////////////////////////////////////////////////////
		// Set rotation. Rebuilds box transform and applies to points.
		void setrot( const vec3 &_rot, float _angle ) {
			raxis = _rot;
			rangle = _angle;
			settransform( pos, scl, raxis, rangle );
		}
		/////////////////////////////////////////////////////////////////////////////
		// Getter for box rotation.
		void getrot( vec3 &_rot, float &_angle ) { _rot = raxis; _angle = rangle; }

		/////////////////////////////////////////////////////////////////////////////
		// Set scale. Rebuilds box transform and applies to points.
		void setscale( const vec3 &_scale ) {
			scl = _scale;
			settransform( pos, scl, raxis, rangle );
		}
		/////////////////////////////////////////////////////////////////////////////
		// Getter for box scale.
		vec3 getscale( void ) { return scl; }

		/////////////////////////////////////////////////////////////////////////////
		// Set velocity.
		void setvel( const vec3 &_velocity ) {
			vel = _velocity;
		}
		/////////////////////////////////////////////////////////////////////////////
		// Getter for velocity.
		vec3 getvel( void ) { return vel; }

		/////////////////////////////////////////////////////////////////////////////
		// Set acceleration.
		void setaccel( const vec3 &_acceleration ) {
			accel = _acceleration;
		}
		/////////////////////////////////////////////////////////////////////////////
		// Getter for acceleration.
		vec3 getaccel( void ) { return accel; }

		/////////////////////////////////////////////////////////////////////////////
		// Takes source points, multiples a mat4 against them, stores result in
		// destination points. Expects 8 points in both source and destination.
		void transformpoints( vec3 *_srcpnts, vec3 *_destpnts, const mat4 &_mat ) {
			// Multiply each point by the given matrix.
			for( int p = 0; p < 8; p++ )
				_destpnts[p] = ( (mat4)_mat * mat4(_srcpnts[p]) ).columns[3];
		}

		/////////////////////////////////////////////////////////////////////////////
		// Copies 8 points from one array to another.
		void copypoints( vec3 *_srcpnts, vec3 *_destpnts ) {
			for( int p = 0; p < 8; p++ )
				_destpnts[p] = _srcpnts[p];
		}

		/////////////////////////////////////////////////////////////////////////////
		// Copies the untransformed points into a given point array.
		void pointsu( vec3 *_points ) {
			copypoints( pntsu, _points );
		}

		/////////////////////////////////////////////////////////////////////////////
		// Copies transformed points into given vec3 array.
		void points( vec3 *_points ) {
			copypoints( pnts, _points );

		}

		/////////////////////////////////////////////////////////////////////////////
		// Checks for a collision between two boxes/cubes.
		// Only collision points for the calling box are generated.
		void collision( PCollision &_pc, const PBox &box2 ) {

			// Initialize collision info first.
			_pc.numcolpnts = 0;
			
			//////////////////
			// Distance Check.
			
				// Before doing ANYTHING, do a simple distance check 
				// to be sure the two boxes are even close enough.
				
				// Get distance between two boxes.
				float dist = sqrt( (box2.pos.x - pos.x) * (box2.pos.x - pos.x) + 
					   			   (box2.pos.y - pos.y) * (box2.pos.y - pos.y) + 
								   (box2.pos.z - pos.z) * (box2.pos.z - pos.z)	);
				
				// If distance is less than sum of max axis', 
				// we have a potential collision.
				if( dist > (largestaxis + box2.largestaxis) )
					return;

			// Distance Check.
			//////////////////
			
			///////////////////////////////
			// 12 lines, 2 collision points.
			vec3 box1cps[12][2];
			// 12 lines, 2 end points.
			vec3 box1lines[12][2];
			// 6 faces with 4 points.
			vec3 box1faces[6][4];
			// 6 xyz normals.
			vec3 box1fnormals[6];
			////////////////////////////////
			// 12 lines, 2 collision points.
			vec3 box2cps[12][2];
			// 12 lines, 2 end points.
			vec3 box2lines[12][2];
			// 6 faces with 4 points.
			vec3 box2faces[6][4];
			// 6 xyz normals.
			vec3 box2fnormals[6];
			///////////////////////////////
			generatelines( pnts, box1lines );
			generatelines( box2.pnts, box2lines );
			generatefaces( pnts, box1faces );
			generatefaces( box2.pnts, box2faces );
			// Use our custom method for generating normals.
			generatefacenormals( box1faces, box1fnormals );
			generatefacenormals( box2faces, box2fnormals );
			///////////////////////////////
			// Loop through all 12 lines and check for collisions.
			// Each line can have a max of 2 collisions, because it's 
			// impossible for there to be more.
			for(int l = 0; l < 12; l++ ) {
				// Initialize the contact/collision points.
				box1cps[l][0] = BADVECTOR;
				box1cps[l][1] = box1cps[l][0];
				box2cps[l][0] = BADVECTOR;
				box2cps[l][1] = BADVECTOR;
				int numbox1cols = 0;
				int numbox2cols = 0;
				// Check every face. If a line collides with two faces, 
				// stop checking for that line.
				for( int f = 0; f < 6; f++ ) {
					box1cps[l][numbox1cols] = lineinface( box1lines[l], box2faces[f] );
					box2cps[l][numbox2cols] = lineinface( box2lines[l], box1faces[f] );
					if( numbox1cols != 2 && ispntvalid(box1cps[l][numbox1cols]) ) {
						_pc.addpoint( 1, f, box1cps[l][numbox1cols], box2faces[f], box2fnormals[f] );
						numbox1cols++;
					}
					if( numbox2cols != 2 && ispntvalid(box2cps[l][numbox2cols]) ) {
						_pc.addpoint( 0, f, box2cps[l][numbox2cols], box1faces[f], box1fnormals[f] );
						numbox2cols++;
					}
					if( numbox1cols == 2 && numbox2cols == 2 )
						break;
				}
			}
		}

		/////////////////////////////////////////////////////////////////////////////
		// Give a triangle and it gives a normalized triangle normal.
		vec3 gettrinormal( const vec3 _tri[3] ) {
			// Use triangle vertices to create a normal.
			vec3 tv1 = (vec3)_tri[0] - (vec3)_tri[1];
			vec3 tv2 = (vec3)_tri[2] - (vec3)_tri[1];
			// Cross and normalize to get a normal.
			return normalize( cross( tv2, tv1 ) );
		}
		
		/////////////////////////////////////////////////////////////////////////////
		// Takes 6 faces and returns all of their normals.
		void generatefacenormals( const vec3 _faces[6][4], vec3 _normals[6] ) {
			// Create a normal for every face.
			for( int f = 0; f < 6; f++ ) {
				// Pull face vertices and create vectors. 
				vec3 fv1 = (vec3)_faces[f][0] - (vec3)_faces[f][1];
				vec3 fv2 = (vec3)_faces[f][2] - (vec3)_faces[f][1];
				// Cross and normalize to get a normal.
				_normals[f] = cross( fv2, fv1 );
				_normals[f] = normalize( _normals[f] );
			}
		}
		

		/////////////////////////////////////////////////////////////////////////////
		// Accepts 8 points(xyz) for a cube and outputs 4 values per face.
		void generatefaces( const vec3 _pnts[8], vec3 _faces[6][4] ) {
			// 1
			_faces[0][0] = _pnts[0];
			_faces[0][1] = _pnts[1];
			_faces[0][2] = _pnts[2];
			_faces[0][3] = _pnts[3];
			// 2
			_faces[1][0] = _pnts[4];
			_faces[1][1] = _pnts[7];
			_faces[1][2] = _pnts[6];
			_faces[1][3] = _pnts[5];
			// 3
			_faces[2][0] = _pnts[5];
			_faces[2][1] = _pnts[6];
			_faces[2][2] = _pnts[1];
			_faces[2][3] = _pnts[0];
			// 4
			_faces[3][0] = _pnts[3];
			_faces[3][1] = _pnts[2];
			_faces[3][2] = _pnts[7];
			_faces[3][3] = _pnts[4];
			// 5
			_faces[4][0] = _pnts[1];
			_faces[4][1] = _pnts[6];
			_faces[4][2] = _pnts[7];
			_faces[4][3] = _pnts[2];
			// 6
			_faces[5][0] = _pnts[5];
			_faces[5][1] = _pnts[0];
			_faces[5][2] = _pnts[3];
			_faces[5][3] = _pnts[4];
			
		}

		/////////////////////////////////////////////////////////////////////////////
		// Takes 8 points for box/cube and  creates 12 lines, each with two xyz's.
		void generatelines( const vec3 _pnts[8], vec3 _lines[12][2] ) {
			for( int l = 0, p = 0; l < 7; l++, p++ ) {
				_lines[l][0] = _pnts[p];
				_lines[l][1] = _pnts[p + 1];
			}
			for( int l2 = 7, p2 = 0; l2 < 12; l2++, p2++ ) {
				_lines[l2][0] = _pnts[p2];
				_lines[l2][1] = _pnts[p2 + 5];
			}
			// 
			_lines[10][0] = _pnts[0];
			_lines[10][1] = _pnts[3];
			// 
			_lines[11][0] = _pnts[7];
			_lines[11][1] = _pnts[4];
			
		}

		/////////////////////////////////////////////////////////////////////////////
		// Takes a face and builds two triangles to test against.
		void generatetris( const vec3 _face[4], vec3 _tris[2][3] ) {
			// Triangle one.
			_tris[0][0] = _face[0];
			_tris[0][1] = _face[1];
			_tris[0][2] = _face[2];
			// Triangle two.
			_tris[1][0] = _face[0];
			_tris[1][1] = _face[2];
			_tris[1][2] = _face[3];
		}

		/////////////////////////////////////////////////////////////////////////////
		// Takes a line (2 * xyz) and a face plane(4 * xyz).
		// Builds two triangles from the given face and
		// does a line to triangle check.
		// Returns point of collision. Point will be
		// vec3(-1000, -1000, -1000) if there was no collision.
		// Can call ispntvalid(vec3()) to determine instead of
		// checking for -1000.
		vec3 lineinface( const vec3 _line[2], const vec3 _face[4] ) {
			generatetris( _face, lit_tris );
			vec3 cpnt = lineintri( _line, lit_tris[0] );
			if( ispntvalid(cpnt) ) return cpnt;
			cpnt = lineintri( _line, lit_tris[1] );
			return cpnt;
		}

		/////////////////////////////////////////////////////////////////////////////
		// Returns a point if a there is a collision between
		// line and triangle. Otherwise returns 
		// vec3(-1000.0f, -1000.0f, -1000.0f). Check with 
		// ispntvalid(vec3()).
		vec3 lineintri( const vec3 _line[2], const vec3 _tri[3] ) {
			
			// Get normal of triangle.
			vec3 trinorm = gettrinormal( _tri );

			// The line's normal/vector.
			vec3 linenorm = (vec3)_line[1] - (vec3)_line[0];
			
			// Save length of line vector/normal.
			float linelen = magnitude( linenorm );

			// Now normalize. it.
			linenorm = normalize( linenorm );
			
			// Dot between start of line and triangle normal.
			float dot_start_offset = dot( _line[0], trinorm );

			// Dot between line normal and triangle normal.
			float dot_lnnorm_trinorm = dot( linenorm, trinorm );

			// Dot of tri normal and first tri vertice.
			float dot_plane_offset = dot( trinorm, _tri[0] );

			// How much to scale the line vector.
			float linescale = ( dot_plane_offset - dot_start_offset ) / ( dot_lnnorm_trinorm ? dot_lnnorm_trinorm : 1 );
			// float linescale = ( dot_plane_offset - dot_start_offset ) / dot_lnnorm_trinorm; 
			
			// If the line scale is less than 0 or greater than 
			// the length of the line, there is no way it's 
			// touching the triangle.
			if( linescale < 0 || linescale > linelen )
				return BADVECTOR;
			
			// Scale line normal.
			linenorm = linenorm * linescale;
			
			// Add scaled normal to line start.
			vec3 checkpoint = (vec3)_line[0] + linenorm;

			// If this point is in the triangle, return it.
			if( pointintri(checkpoint, _tri) )
				return checkpoint;
			
			// If we made it here, the line is NOT colliding with the triangle.
			return BADVECTOR;
		}

		/////////////////////////////////////////////////////////////////////////////
		// Checks for collision/intersection between point and triangle.
		bool pointintri( const vec3 &_pnt, const vec3 _tri[3] ) {

			// Sum of angles between vectors from point to tri vertices.
			float degs = 0.0f;

			// Vectors between point and tri vertices.
			vec3 v1 = (vec3)_pnt - (vec3)_tri[0];
			vec3 v2 = (vec3)_pnt - (vec3)_tri[1];
			vec3 v3 = (vec3)_pnt - (vec3)_tri[2];

			// Normalize the vectors.
			v1 = normalize( v1 );
			v2 = normalize( v2 );
			v3 = normalize( v3 );
			
			// Add up angles between vectors.
			degs += acos( dot(v1, v2) );
			degs += acos( dot(v2, v3) );
			degs += acos( dot(v3, v1) );
			
			// If the sum of the angles is 2 * PI, the point is in the triangle.
			if( fabs(degs - 2 * 3.141592654) < 0.005f )
				return true;

			// No collision.
			return false;
		}

		/////////////////////////////////////////////////////////////////////////////
		//
		bool ispntvalid( const vec3 &_pnt ) {
			// Might just check .x later.
			return ( (_pnt.x != -1000.0f) &&
					 (_pnt.y != -1000.0f) &&
					 (_pnt.y != -1000.0f) ) ? true : false;
		}
		
		/////////////////////////////////////////////////////////////////////////////
		// 
		// Creates a quaternion from an axis-angle.
		// Make sure axis is normalized.
		// Takes vec3 and degrees.
		vec3 axis2quat( const vec3 &_axis, float _angle ) {
			_angle = (_angle * 3.141592f) / 180.0f;
			return vec3( _axis.x * sin( _angle / 2 ), // qx
						 _axis.y * sin( _angle / 2 ), // qy
						 _axis.z * sin( _angle / 2 ), // qz
						 cos(_angle / 2) );			  // qw
		}
		/////////////////////////////////////////////////////////////////////////////
		// 
		// Takes a "quaternion" and returns axis-angle.
		// Our _quat is just a vec3
		// Returns axis and angle(in degrees).
		void quat2axis( const vec3 &_quat, vec3 &_axis, float &_angle ) {
			_angle = ( (2 * acos( _quat.w )) * 180.0f ) / 3.141592f;
			_axis.x = _quat.x / sqrt( 1 - _quat.w * _quat.w );
			_axis.y = _quat.y / sqrt( 1 - _quat.w * _quat.w );
			_axis.z = _quat.z / sqrt( 1 - _quat.w * _quat.w );
			_axis = normalize( _axis );
		}
		/////////////////////////////////////////////////////////////////////////////
		//
		void fixpenetration( const PCollision &pc ) {
			// Get the average normal from all faces involved.
			vec3 box1avgnorm;
			vec3 box2avgnorm;
			((PCollision)pc).averagenormals1f( box1avgnorm, box2avgnorm );
			// A bit of a hack. It's possible for the first box to be "bigger" than
			// the second. This can introduce a scenario where NONE of box1 lines are
			// are touching box2 faces, which generates NO box2 face collisions.
			// Here we choose to use box1 faces/normals if box2 doesn't have anything
			// useful.
			vec3 anscaled = ( magnitude(box2avgnorm) > 0 ? box2avgnorm : box1avgnorm * -1);
			anscaled = normalize( anscaled );
			//anscaled = anscaled * ( (pc.numcolpnts < 6) ? 0.1f : 0.05f );
			// Apply to this box's position.
			setpos( pos + anscaled * 0.02f );
			// setpos( pos - vel );
		}

		/////////////////////////////////////////////////////////////////////////////
		// 
		// Multiply Axis-Angle
		// xyz w=angle in degrees.
		// The order a1/2 are passed is important.
		// Returns vector with xyz rotation axis and w is angle in degrees.
		vec3 multaa( vec3 a1, vec3 a2 ) {
			// Create rotation matrices and combine.
			float fa1 = a1.w; a1.w = 1;
			float fa2 = a2.w; a2.w = 1;
			mat4 a1mat = rotate( fa1, a1 );
			mat4 a2mat = rotate( fa2, a2 );
			mat4 cmmat = a1mat * a2mat;
			
			// Convert back to axis angle.

			// Clamp this angle value before passing to acos. Will get NaN otherwise.
			float acosval = (cmmat[0][0] + cmmat[1][1] + cmmat[2][2] - 1) / 2;
			acosval = acosval > -1 ? acosval : -0.999f;
			acosval = acosval < 1 ? acosval : 0.999;
			
			// Calc angle.
			float newangle = acos( acosval );
			// Calc square root and clamp.
			float sqr = sqrt( (cmmat[2][1] - cmmat[1][2]) * (cmmat[2][1] - cmmat[1][2]) +
							  (cmmat[0][2] - cmmat[2][0]) * (cmmat[0][2] - cmmat[2][0]) +
							  (cmmat[1][0] - cmmat[0][1]) * (cmmat[1][0] - cmmat[0][1]) );
			if( fabs(sqr) < 0.001f ) sqr = 1.0f;
			
			// Store axis values and angle.
			vec3 newraxis;
			newraxis.x = ( cmmat[2][1] - cmmat[1][2] ) / sqr;
			newraxis.y = ( cmmat[0][2] - cmmat[2][0] ) / sqr;
			newraxis.z = ( cmmat[1][0] - cmmat[0][1] ) / sqr;
			newraxis = newraxis * -1;
			newangle = (newangle * 180.0f) / 3.141592f;
			if( newangle >= 360 )  newangle = 0;
			if( newangle <= -360 ) newangle = 0;
			newraxis.w = newangle;

			// Finally return new axis angle.
			return newraxis;
		}

		/////////////////////////////////////////////////////////////////////////////
		//
		void reaction( const PCollision &pc ) {
			// Get average contact point.
			vec3 avgpnt = ((PCollision)(pc)).averagepoint();
			// Get vector from box center-point to
			// contact point.
			vec3 contactvector = normalize( avgpnt - pos );
			// Cross contact vector and velocity to get a rotation vector.
			vec3 rotvector = normalize( cross(contactvector, normalize(vel)) );
			// Get angle between contact point vector and
			// velocity vector.
			float vangle = acos( dot(contactvector, normalize(vel)) );
	
			// Prepare vectors for multaa().
			rotvector.w = ((vangle * -0.05f) * 180.0f) / 3.141592f;
			vec3 tmpraxis = raxis;
			tmpraxis.w = rangle;
			vec3 naxis = multaa( rotvector, tmpraxis );
			float nangle = naxis.w;
			naxis.w = 1;

			// Store for when there isn't a collision later.
			lastrotaxis = rotvector;
			lastrotangle = ((vangle * -0.05f) * 180.0f) / 3.141592f;

			// Set rotation to new rotation.
			setrot( naxis, nangle );
		}

		/////////////////////////////////////////////////////////////////////////////
		// 
		void applylastrot( void ) {
			if( lastrotangle ) {
				// Prepare vectors for multaa().
				vec3 v1 = lastrotaxis;
				v1.w = lastrotangle * magnitude( vel );
				vec3 v2 = raxis;
				v2.w = rangle;
				// Multiply them together.
				vec3 naxis = multaa( v1, v2 );
				// Set new rotation.
				float nangle = naxis.w;
				naxis.w = 1;
				setrot( naxis, nangle );
			}
		}

		/////////////////////////////////////////////////////////////////////////////
		// 
		// Updates all box's velocities, positions, etc.
		// Bonus! It will also handle collisions for you.
		// Note: Call every iteration and you MUST pass a box and 
		// a number. 
		// Another Note: DOESN'T CHECK POINTER OR NUMBOXES!
		// Example 1: 
		// PBox boxes[2];
		// PBox::update( boxes, 2 );
		// 
		// Example 2:
		// PBox box( vec3(0, 0, 0) );
		// PBox::update( &box, 1 );
		// 
		static void update( PBox *pboxes, int _numboxes ) {

			// Update every box's vel/pos/etc.
			for( int pb = 0; pb < _numboxes; pb++ ) {
				// Update velocity.
				pboxes[pb].vel = pboxes[pb].vel + pboxes[pb].accel;
				// Update position.
				pboxes[pb].setpos( pboxes[pb].pos + pboxes[pb].vel );
			}

			// Do collision/reaction for every box.
			for( int b = 0; b < _numboxes; b++ ) {
				for( int c = b; c < _numboxes - 1; c++ ) {
					// Check for a collision.
					pboxes[b].collision( pboxes[b].pc, pboxes[c + 1] );
					// React to this collision.
					if( pboxes[b].pc.numcolpnts > 0 ) {
						// Fix penetration and react for box 1. 
						if( pboxes[b].dynamic ) {
							pboxes[b].fixpenetration( pboxes[b].pc );
							pboxes[b].reaction( pboxes[b].pc );
						}
						// Grab collision info for second box.
						if( pboxes[c + 1].dynamic ) {
							pboxes[c + 1].collision( pboxes[c + 1].pc, pboxes[b] );
							if( pboxes[c + 1].pc.numcolpnts > 0 ) {
								pboxes[c + 1].fixpenetration( pboxes[c + 1].pc );
								pboxes[c + 1].reaction( pboxes[c + 1].pc );
							}
							else {
								pboxes[c + 1].applylastrot();
							}
						}

					} // if( pc.numcolpnts...
					else {
						pboxes[b].applylastrot();
					}

				} // for( int c...

			} // for( int b...


		} // update()
};
