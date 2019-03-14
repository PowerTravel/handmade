
import numpy as np
from numpy import linalg as LA
import matplotlib.pyplot as plt
import matplotlib.cm as cm
from mpl_toolkits.mplot3d import Axes3D


def isInside( v ):
	p0 = np.array([0,0,0])
	p1 = np.array([1,0,0.5])
	p2 = np.array([1,1,1])
	p3 = np.array([0,1,0.5])

	E0 = p1-p0
	E1 = p2-p1
	E2 = p3-p2
	E3 = p0-p3

	v0 = v-p0
	v1 = v-p1
	v2 = v-p2
	v3 = v-p3

	c0 = np.cross(E0,v0)
	c1 = np.cross(E1,v1)
	c2 = np.cross(E2,v2)
	c3 = np.cross(E3,v3)

	surfaceNormal =  np.cross(E0,E1) / LA.norm( np.cross(E0,E1) )
	cm = (p1+p2+p3+p0)/4
	origin = [0,0,0]
	X, Y, Z = zip(p0,p1,p2,p3,p0,p1,p2,p3,p0,p1,p2,p3, cm)
	A, B, C = zip(E0,E1,E2,E3,c0,c1,c2,c3,v0,v1,v2,v3, surfaceNormal )

	fig = plt.figure()

	ax = fig.add_subplot(111, projection='3d')

	ax.set_xlim3d(-2,2)
	ax.set_ylim3d(-2,2)
	ax.set_zlim3d(-2,2)

	print(np.dot(c0,surfaceNormal))
	print(np.dot(c1,surfaceNormal))
	print(np.dot(c2,surfaceNormal))
	print(np.dot(c3,surfaceNormal))

	ax.quiver(X,Y,Z,A,B,C)
	plt.show()


#v = np.array([0.5,0.5,0.5])
#v = v - 0.1 * np.array([-1,-1,1])
#isInside(v)

def Normalize( v ):
	Result = v / LA.norm(v);
	return Result

def VertexEdge( Vertex, EedgePointA, EdgePointB):
	o = EedgePointA;
	d = EdgePointB;
	uNorm = LA.norm(d-o);
	u = Normalize(d-o);
	v = Vertex;

	ProjectionScalar = np.dot( (v-o), u);
	print("VertexEdge ProjectionScalar: ", ProjectionScalar)
	if ProjectionScalar <= 0:
		ClosestPointOnEdge = o;

	elif ProjectionScalar >= uNorm:
		
		ClosestPointOnEdge = d;

	else:
		ClosestPointOnEdge = o + ProjectionScalar * u;
	
	return v, ClosestPointOnEdge;

def EdgeEdge( o1, d1, o2, d2 ):

	u1 = Normalize(d1-o1);
	u2 = Normalize(d2-o2);
	k = np.dot(u1,u2)
	
	Tol = 10E-7;

	if( np.abs(np.abs(k)-1) >= Tol ):
		print("Lines are not parallel")
		Scalar = np.dot( (o2-o1), (u1 - k * u2) ) / (1 - k*k);

		ClosestPointOnRay = o1 + Scalar * u1;
		
		no1 = ClosestPointOnRay - o1;
		so1 = np.dot( no1, u1 );
		nd1 = ClosestPointOnRay - d1;
		sd1 = np.dot( nd1, u1 );

		no2 = ClosestPointOnRay - o2;
		so2 = np.dot( no2, u2 );
		nd2 = ClosestPointOnRay - d2;
		sd2 = np.dot( nd2, u2 );


		IsOnLineSegmentA = ( (so1 >= 0) and (sd1 <= 0) );

		IsOnLineSegmentB = ( (so2 >= 0) and (sd2 <= 0) );

		if IsOnLineSegmentA==True and IsOnLineSegmentB == True:
		
			print("Closest point is on both line segments")
			P0,P1 = VertexEdge( ClosestPointOnRay, o2, d2 );

		else:
			
			print("Closest point outside one or both of the segments")
			A0B0,A0B1 = VertexEdge( o1, o2, d2);
			ShortestContactDistance = LA.norm( A0B1 - A0B0 );
			print(ShortestContactDistance)
			print(o1, o2, d2)
			print("Start with:")
			fig = plt.figure()
			PlotArrows( np.array([o1[0],d1[0], o2[0], d2[0], A0B0[0], A0B1[0]]),
						np.array([o1[1],d1[1], o2[1], d2[1], A0B0[1], A0B1[1]]),
						np.array([o1[2],d1[2], o2[2], d2[2], A0B0[2], A0B1[2]]))
			
			P0 = A0B0 
			P1 = A0B1;

			A1B0, A1B1 = VertexEdge( d1,   o2, d2);
			ContactDistance = LA.norm( A1B1 - A1B0 );
			print( ContactDistance )

			if( ContactDistance < ShortestContactDistance ):
				ShortestContactDistance = ContactDistance;
				P0 = A1B0 
				P1 = A1B1;
				print("Better")

			PlotArrows( np.array([o1[0],d1[0], o2[0], d2[0], A1B0[0], A1B1[0]]),
						np.array([o1[1],d1[1], o2[1], d2[1], A1B0[1], A1B1[1]]),
						np.array([o1[2],d1[2], o2[2], d2[2], A1B0[2], A1B1[2]]))
	



			B0A0,B0A1 = VertexEdge( o2, o1, d1);
			ContactDistance = LA.norm( B0A1 - B0A0 );
			print( ContactDistance )
			if( ContactDistance < ShortestContactDistance ):
				ShortestContactDistance = ContactDistance;
				P0 = B0A1;
				P1 = B0A0;
				print("Better")

			PlotArrows( np.array([o1[0],d1[0], o2[0], d2[0], B0A0[0], B0A1[0]]),
						np.array([o1[1],d1[1], o2[1], d2[1], B0A0[1], B0A1[1]]),
						np.array([o1[2],d1[2], o2[2], d2[2], B0A0[2], B0A1[2]]))
	
			


			B1A0,B1A1 = VertexEdge( d2,   o1, d1);
			ContactDistance = LA.norm( B1A1 - B1A0 );
			print( ContactDistance )

	
			if( ContactDistance < ShortestContactDistance ):
				ShortestContactDistance = ContactDistance;
				P0 = B1A1;
				P1 = B1A0;
				print("Better")

			PlotArrows( np.array([o1[0],d1[0], o2[0], d2[0], B1A0[0], B1A1[0]]),
						np.array([o1[1],d1[1], o2[1], d2[1], B1A0[1], B1A1[1]]),
						np.array([o1[2],d1[2], o2[2], d2[2], B1A0[2], B1A1[2]]))
	else:
		print("Lines are parallel")
		if(k < 0):
			TmpPoint = o2;
			o2 = d2;
			d2 = TmpPoint;

		A0B0,A0B1 = VertexEdge( o1, o2, d2);
		A0Bn = A0B1-A0B0;
		A0Bp = np.abs( np.dot(A0Bn, u1) ) < Tol;

		A1B0,A1B1 = VertexEdge( d1,   o2, d2);
		A1Bn = A1B1-A1B0;
		A1Bp = np.abs( np.dot(A1Bn, u1) ) < Tol;

		B0A0,B0A1 = VertexEdge( o2, o1, d1);
		B0An = B0A1-B0A0;
		B0Ap = np.abs( np.dot(B0An, u2) ) < Tol;

		B1A0,B1A1 = VertexEdge( d2,   o1, d1);
		B1An = B1A1-B1A0;
		B1Ap = np.abs( np.dot(B1An, u2) ) < Tol;

		if( not (A0Bp or A1Bp or B0Ap or B1Ap) ):
		
			# Separate:
			#           |--A--|
			#  |--B--|

			print("Lines are separate")
			A0BMinDist = LA.norm( A0Bn );
			A1BMinDist = LA.norm( A1Bn );
			if( A0BMinDist < A1BMinDist ):
				P0 = A0B0;
				P1 = A0B1;
			else:
				P0 = A1B0;
				P1 = A1B1;

		elif (A0Bp and A1Bp):
			print("Lines 1 is inside 2")
			#	A inside B
			#      |----A----|
			#  |--------B--------|
			P0 = (A0B0 + A1B0)/2;
			P1 = (A0B1 + A1B1)/2;
		elif (B0Ap and B1Ap):
			print("Lines 2 is inside 1")
			#  B inside A
			#  |--------A--------|
			#      |----B----|
			P0 = (B0A1 + B1A1)/2;
			P1 = (B0A0 + B1A0)/2;
		elif(A0Bp and B1Ap):
			print("Lines 2 is to the left of line 2")
			# B to the left of A
			#      |----A----|
			# |----B----|
			P0 = (A0B0 + B1A1)/2;
			P1 = (A0B1 + B1A0)/2;
		elif(B0Ap and A1Bp):
			print("Lines 2 is to the right of line 2")
			# B to the Right of A
			# |----A----|
			#      |----B----|
			P0 = (B0A1 + A1B0)/2; 
			P1 = (B0A0 + A1B1)/2;

	return P0,P1;

def PlotArrows( X, Y, Z ):

	fig = plt.figure()
	idx = 111


	A = np.array( [X,Y,Z] )
	MinAxis = np.amin( A, axis=1)
	MaxAxis = np.amax( A, axis=1)
	
	ax = fig.add_subplot(idx, projection='3d')

	dim = np.array( [(MaxAxis[0] - MinAxis[0] ), (MaxAxis[1] - MinAxis[1] ) , (MaxAxis[2] - MinAxis[2] ) ])
	m = np.amax(dim)/2;
	center = np.array( [(MaxAxis[0] + MinAxis[0] )/2, (MaxAxis[1] + MinAxis[1] )/2 , (MaxAxis[2] + MinAxis[2] )/2 ])

	ax.set_xlim3d( center[0] - m, center[0] + m )
	ax.set_ylim3d( center[1] - m, center[1] + m )
	ax.set_zlim3d( center[2] - m, center[2] + m )

	X0 = X[::2];
	Y0 = Y[::2];
	Z0 = Z[::2];
	X1 = X[1::2];
	Y1 = Y[1::2];
	Z1 = Z[1::2];

	ax.quiver( X0, Y0, Z0, X1-X0, Y1-Y0, Z1-Z0)

	plt.show()	


def PlotEdgeEdge(o1, d1, o2, d2):
	P0, P1 = EdgeEdge( o1, d1, o2, d2);
	print(P0,P1)
	PlotArrows( np.array([o1[0],d1[0], o2[0],d2[0], P0[0], P1[0]]),
				np.array([o1[1],d1[1], o2[1],d2[1], P0[1], P1[1]]),
				np.array([o1[2],d1[2], o2[2],d2[2], P0[2], P1[2]]))
	




v = np.array(  [ 2, 5, 5] );
o2 = np.array( [-4, 10, 5] )
d2 = np.array( [-8, 6, 5] )
A,B = VertexEdge( v, o2, d2)
PlotArrows( np.array([0, v[0], o2[0],d2[0], A[0], B[0] ]),
			np.array([0, v[1], o2[1],d2[1], A[1], B[1] ]),
			np.array([0, v[2], o2[2],d2[2], A[2], B[2] ]))



o1 = np.array( [ -1,-1,-1] );
d1 = np.array( [  1, 1, 1] );
o2 = np.array( [ -1, 2, 1] )
d2 = np.array( [  1,-0,-1] )
#PlotEdgeEdge(o1, d1, o2, d2)




A0  = np.array([-2,  5,  5]);
A1  = np.array([ 2,  5,  5]);
A0H = np.array([-2,  5,  6]);
A1H = np.array([ 2,  5,  6]);

#     B0(S)
#    /|
#   / |
# B1S B1
#        A0----A1
B0  = np.array([ -4, 10, 5]);
B1  = np.array([ -4,  6, 5]);
B0S = np.array([ -4, 10, 5]);
B1S = np.array([ -8,  6, 5]);
