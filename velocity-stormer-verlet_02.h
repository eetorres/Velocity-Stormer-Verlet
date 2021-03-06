/*
   Filename:		velocity-stormer-verlet_02.h
   Calling:			#include "velocity-stormer-verlet_02.h"
   Application:		contains functions to implement various molecular-dynamics
					processes such as VSV integration, potential energy calculations
					and memory declarations for large bodies of particles.
   Author:			R.O. Ocaya		
   Copyright:		(c) 2015 University of the Free State (UFS).
   First created:	04 April 2015
   Last modified:	Wednesday 19 August 2015.

   Note:	The printf() function consumes the most simulation time if included. 
			It is included in actual calculation functions only during program 
			development, and only for final program output.
*/
#include "sim_constants.h"
#define DIM 3
#define sqr(x) ((x)*(x))

char str1[] = {"mass"};
char str2[] = {"x"};
char str3[] = {"y"};
char str4[] = {"z"};
char str5[] = {"x_vel"};
char str6[] = {"y_vel"};
char str7[] = {"z_vel"};

FILE *outputFile;

// particle attributes
typedef struct{
	double m;			// mass
	double x[DIM];		// position
	double v[DIM];		// velocity
	double F[DIM];		// force
	double F_old[DIM];	// the old force
} Particle;				// each particle uses 72 bytes

// Defines a ParticleList structure for the Linked Cell Method (LCM).
// The ParticleList structure contains a particle type and a pointer
// to another structure of the same type.
typedef struct ParticleList{
	Particle p;					// define the particle
	struct ParticleList *next;	// pointer to another particle
} ParticleList;

typedef ParticleList* Cell;		// define a new type called "Cell".

// function prototypes
void timeIntegration_basis (double t, double delta_t, double t_end, Particle *p, int N);
void outputResults_basis(Particle *p, double N, double t);
void computeF_basis(Particle *p, int N);
void computeF2_basis(Particle *p, int N);
void force(Particle *i, Particle *j);
void force2(Particle *i, Particle *j);  // based on the modified algorithm
void computeX_basis(Particle *p, int N, double delta_t);
void computeV_basis(Particle *p, int N, double delta_t);
void updateX(Particle  *p,  double delta_t);
void updateV(Particle *p, double delta_t);
void compoutStatistic_basis(Particle *p, int N, double t);
void particleFileContent(Particle *p, int pcount);
void openOutputFile(void);
double Utot(Particle *p, int N);
double Utot2(Particle *p, int N);
void insertList(ParticleList **root_list, ParticleList *i);
void deleteList(ParticleList **q);
void computeF_LC(Cell *grid, int *nc, float r_cut);
void computeX_LC(Cell *grid, int *nc, float *l, float delta_t);
void computeV_LC(Cell *grid, int *nc, float *l, float delta_t);
void moveParticles_LC(Cell *grid, int *nc, float *l);
double invRootS(Particle *p, int k, int N);
void F_i(Particle *p, int i, int N);

void insertList(ParticleList **root_list, ParticleList *i){
	i->next = *root_list;
	*root_list = i;
}

void deleteList(ParticleList **q){
	*q = (*q)->next;	// (*q)->next points to element to be removed
}

void computeF_LC(Cell *grid, int *nc, float r_cut)
{
	int ic[DIM], kc[DIM];
	int d;
	double r;
	ParticleList *i, *j;
	
	for (ic[0]=0; ic[0]<nc[0]; ic[0]++)
	for (ic[1]=0; ic[1]<nc[1]; ic[1]++)
	for (ic[2]=0; ic[2]<nc[2]; ic[2]++)
	
	for (i = grid[index(ic,nc)]; NULL!=i; i=i->next)
    {
	  for (d=0; d<DIM; d++)
		i->p.F[d] = 0;
	  for (kc[0]=ic[0]-1; kc[0]<=ic[0]+1; kc[0]++)
	  for (kc[1]=ic[1]-1; kc[1]<=ic[1]+1; kc[1]++)
	  for (kc[2]=ic[2]-1; kc[2]<=ic[2]+1; kc[2]++)
		
		{ //treat kc[d]<0 and kc[d]>=nc[d] according to boundary conditions;
				// if (distance of i->p to cell kc <= r_cut)
				if (1)
					for (j = grid[index(kc,nc)]; NULL!=j; j=j->next)
						if (i!=j)
						{
							r = 0;
							for (d=0; d<DIM; d++)
								r += sqr(j->p.x[d] - i->p.x[d]);
							if (r<=sqr(r_cut))
							  force2(&i->p, &j->p);
						}
		 }
    }
}

void computeX_LC(Cell *grid, int *nc, float *l, float delta_t){
	int ic[DIM];
	ParticleList *i;
	
	for (ic[0]=0; ic[0]<nc[0]; ic[0]++)
		for (ic[1]=0; ic[1]<nc[1]; ic[1]++)
			for (ic[2]=0; ic[2]<nc[2]; ic[2]++)
				for (i = grid[index(ic,nc)]; NULL!=i; i=i->next)
					updateX(&i->p, delta_t);
	moveParticles_LC(grid, nc, l);
}

void computeV_LC(Cell *grid, int *nc, float *l, float delta_t){
	int ic[DIM];
	ParticleList *i;

	for (ic[0]=0; ic[0]<nc[0]; ic[0]++)
		for (ic[1]=0; ic[1]<nc[1]; ic[1]++)
			for (ic[2]=0; ic[2]<nc[2]; ic[2]++)
				for (i = grid[index(ic,nc)]; NULL!=i; i=i->next)
					updateV(&i->p, delta_t);
}
  
void moveParticles_LC(Cell *grid, int *nc, float *l){
	int ic[DIM], kc[DIM];
	int d;
	ParticleList *i, *q;
	
	for (ic[0]=0; ic[0]<nc[0]; ic[0]++)
		for (ic[1]=0; ic[1]<nc[1]; ic[1]++)
			for (ic[2]=0; ic[2]<nc[2]; ic[2]++){
				ParticleList **q = &grid[index(ic,nc)]; // pointer to predecessor
				ParticleList *i = *q;
				
				while (NULL != i){
					// treat boundary conditions for i->x;
					for (d=0; d<DIM; d++)
						kc[d] = (int)floor(i->p.x[d] * nc[d] / l[d]);
					if ((ic[0]!=kc[0])||(ic[1]!=kc[1])||(ic[2]!=kc[2])){
						deleteList(q);
						insertList(&grid[index(kc,nc)], i);
					}else q = &i->next;
					i = *q;
				}
			}
}


void timeIntegration_basis (double t, double delta_t, double t_end, Particle *p, int N)
{
	computeF_basis(p, N);					// find the initial force
	openOutputFile();					// open disk file for output

	while (t < t_end){
		t += delta_t;
		computeX_basis(p, N, delta_t);
		computeF_basis(p, N);
		computeV_basis(p, N, delta_t);
		compoutStatistic_basis(p, N, t);
	
		outputResults_basis(p, N, t);	// print values at each time step
	}	
	fclose(outputFile);
}

// this next integration is based on the modified algorithm VSV algorithm
void timeIntegration2_basis (double t, double delta_t, double t_end, Particle *p, int N)
{
	computeF2_basis(p, N);				// find the initial force
	openOutputFile();				// open disk file for output

	while (t < t_end){
		t += delta_t;
		computeX_basis(p, N, delta_t);
		computeF2_basis(p, N);			// uses the modified algorithm
		computeV_basis(p, N, delta_t);
		compoutStatistic_basis(p, N, t);
	
		outputResults_basis(p, N, t);	// print values at each time step
	}	
	fclose(outputFile);
}

void openOutputFile(void){
	if ((outputFile = fopen("outdata.txt", "w")) == NULL){
		printf("Error: cannot open file for writing...\n");
		exit (0);
	}
}

void outputResults_basis(Particle *p, double N, double t){
	// int i;
	fprintf(outputFile, "%1.3e %1.3e %1.3e %1.3e %1.3e %1.3e %1.3e %1.3e\n", t, p[3].m, \
				p[3].x[0], p[3].x[1], p[3].x[2], p[3].v[0], p[3].v[1], p[3].v[2]);
}

/*
The following function uses the naive force computation algorithm, force();
*/
void computeF_basis(Particle *p, int N){
	int i, d, j;
	for (i=0; i<N; i++)
	  for (d=0; d<DIM; d++)
		  p[i].F[d]=0;
	for (i=0; i<N; i++)
	  for (j=0; j<N; j++)
		  if (i!=j) force (&p[i],&p[j]);
}

/*
The following uses the modified force algorithm, force2() to compute the force;
*/
void computeF2_basis(Particle *p, int N){
	int i, d, j;
	for (i=0; i<N; i++)
	  for (d=0; d<DIM; d++)
		  p[i].F[d]=0;
	for (i=0; i<N; i++)
	  for (j=i+1; j<N; j++)    // note that j = i+1;
		  if (i!=j) force2 (&p[i],&p[j]);
}

/* 
   This algorithm calculates force in a naive way. The force is a 
   gravitational force. It is sufficient to modify just this part
   for forces based on other potentials, such as the Lennard-Jones
*/
void force(Particle *i, Particle *j){
	int d;
	double f, r = 0;
	
	for (d=0; d<DIM; d++)
	r += sqr(j->x[d]-i->x[d]);
	f = (i->m * j->m)/(sqrt(r)*r);
	
	for (d=0; d<DIM; d++)
		i->F[d] += f*(j->x[d]- i->x[d]);
}

/* 
   This is the modified algorithm, see p52 of Griebel, with the 
   gravitational force example. Modify just this part for other 
   potentials, e.g. 12-6 Lennard-Jones.
*/
void force2(Particle *i, Particle *j){
	int d;
	double f, r = 0;
	
	for (d=0; d<DIM; d++)
	r += sqr(j->x[d]-i->x[d]);
	f = (i->m * j->m)/(sqrt(r)*r);         // denominator is rij^3
	
	for (d=0; d<DIM; d++){
		i->F[d] += f*(j->x[d]- i->x[d]);  // this is Fij
		j->F[d] -= f*(j->x[d]- i->x[d]);  // this is Fji = -Fij
	}
}

/*
	Function returns reciprocal of square root of the intermediate sum S i.e. 
	1/S_k used in force calculation in the Finnis-Sinclair EAM model according 
	to Sutton-Chen. See Griebel Eq. 5.5, p154. 
*/
double invRootS(Particle *p, int k, int N){
	int j, d;
	double r, val = 0;
		
	for (j=0; j<N; j++){					
		if (j!=k){
			r=0;
			for (d=0; d<DIM; d++)
				r += sqr(p[j].x[d]-p[k].x[d]);	// r^2 = dx^2 + dy^2 + dz^2
			r = pow(r,0.5);						// r = sqrt(r^2)
			val += pow(lat_const/r, mint);		// sum (N-1) terms excluding j=k;
		}
	}
	// printf("S_%d = %.4f\n", k, val); // for debugging only
	val = 1/pow(val,0.5);						// return reciprocal of square root
	// printf("\ninvRootS_%d = %.4f\n", k, val); // for debugging only
	return (val);
}

/*
	Function receives particle array and returns force on the i-th particle using 
	Finnis-Sinclair EAM model according to Sutton-Chen, see Griebel Eq. 5.5, p154. 
*/
void F_i(Particle *p, int i, int N){
	int j, d;
	double r, r2, rn, f;
	double iterm, jterm;
	
	for (j=0; j<N; j++){					
		if (j!=i){
			r=0;
			for (d=0; d<DIM; d++)				// determine rij for each i and j pair
				r += sqr(p[j].x[d]-p[i].x[d]);	// r^2 = dx^2 + dy^2 + dz^2
			r2 = r;								// r2 = r^2
			r = pow(r,0.5);						// rij = sqrt(r^2)
			rn = lat_const/r;					// rn = (sigma/rij) in Griebel	
			iterm = invRootS(p,i,N);
			jterm = invRootS(p,j,N);
			f = -eps*(nint*pow(rn,nint)-0.5*cn*mint*(iterm + jterm)*pow(rn, mint))/r2;
			// printf("i = %d, S_i = %.4f, S_j = %.4f\n", i, iterm, jterm); // for debugging only
			printf("\nf_%d = %.4f\n", j, f); // for debugging only

			/*	for each i, update sum up forces Fij for each dimension
				and store the force components */ 			
			for (d=0; d<DIM; d++)				
				p[i].F[d] += f * (p[j].x[d] - p[i].x[d]); 
		}
	}
}

/* 
   Function computes the Lennard-Jones 12-6 pairwise particle force.
   See page 54 of Griebel et al, Eq. 3.28 (p54).
*/
void force_LJ(Particle *i, Particle *j){
	double r, s, f = 0;
	int d;
	for (d=0; d<DIM; d++)
		r += sqr(j->x[d] - i->x[d]);		// here r=sqr(rij)
	s = sqr(lat_const) / r;
	s = sqr(s) * s;							// here s=pow(σ/rij,6)
	f = 24 * eps * s / r * (1 - 2 * s);
	
	for (d=0; d<DIM; d++)
		i->F[d] += f * (j->x[d] - i->x[d]); // store force components
}

void computeX_basis(Particle *p, int N, double delta_t){
	int i;
	for  (i=0;  i<N;  i++)
		updateX(&p[i],  delta_t);
}

void computeV_basis(Particle *p, int N, double delta_t){
	int i;
	for  (i=0;  i<N;  i++)
		updateV(&p[i],  delta_t);
}

void updateX(Particle  *p,  double delta_t){
	int d;
	double  a = delta_t * .5 / p->m;
	for  (d=0;  d<DIM;  d++){
		p->x[d] += delta_t*(p->v[d] +  a * p->F[d]);	// according to (3.22)
		p->F_old[d]  =  p->F[d];
}
}

void updateV(Particle *p, double delta_t){
	int d;
	double a = delta_t * .5 / p->m;
	for  (d=0; d<DIM; d++)
		p->v[d] += a * (p->F[d] + p->F_old[d]);			// according to (3.24)
}

void compoutStatistic_basis(Particle *p, int N, double t){
	double v, e = 0;
	int i, d;

	for (i=0; i<N; i++){
		v = 0;
		for (d=0; d<DIM; d++)
			v += sqr(p[i].v[d]);
		e += .5 * p[i].m * v;
}
// print kinetic energy e at time t
}

void particleFileContent(Particle *p, int pcount){
	int i;
	
	printf("\nParticle data from file:\n");
	printf("\n%10s %10s %10s %10s %10s %10s %10s\n", str1, str2, str3, str4, str5, str6, str7);
	
	for (i=0; i<pcount; i++)
		printf("%1.3e %1.3e %1.3e %1.3e %1.3e %1.3e %1.3e\n", p[i].m, \
				p[i].x[0], p[i].x[1], p[i].x[2], p[i].v[0], p[i].v[1], p[i].v[2]);
}
/*
	Function Utot() computes the total potential energy in the particle system.
	Molecular dynamics model: Sutton-Chen Embedded Atom Method (EAM).
	Function parameters: 
	1.	particle array in memory
	2.	number of particles.
*/
double Utot(Particle *p, int N){
	double rho_i=0, V_i=0, a_sum=0, b_sum=0, U_sum=0;
	double r=0;
	int i, j, d;

	for (i=0; i<N; i++){ 						// outer summation of rho for all N
		a_sum = 0;								// reset inner_sum for each i value
		for (j=0; j<N; j++){					
			if (j!=i){
			r=0;
			for (d=0; d<DIM; d++)
				r += sqr(p[j].x[d]-p[i].x[d]);	// find rij
			r = pow(r,0.5);
			// printf("\nr%d_%d=%.4f", i, j, r); // interatomic distances, for debugging only
												// note: printf() consumes the most simulation time if included
			a_sum += pow(lat_const/r, nint);	// sum for (N-1) terms ie exclude j=i;
			}
		}
		a_sum = 0.5*a_sum;
	
		b_sum = 0;								// reset inner sum for each i value
		for (j=0; j<N; j++){					
			if (j!=i){
			r=0;
			for (d=0; d<DIM; d++)
				r += sqr(p[j].x[d]-p[i].x[d]);	// r^2 = dx^2 + dy^2 + dz^2
			r = pow(r,0.5);						// r = sqrt(r^2)
			// printf("\nr%d_%d=%.4f", i, j, r);	// interatomic distances, for debugging only
			b_sum += pow(lat_const/r, mint);	// sum for (N-1) terms ie exclude j=i;
			}
		}
		b_sum = cn*sqrt(b_sum);					// find sqrt of inner sum
		printf("\nE%d = %.4f", i, a_sum-b_sum);	// for debugging only
		U_sum += (a_sum - b_sum);	// update U_sum
	}
	printf("\nRepulsive energy = %.2e (eV), Cohesive energy = %.2e (eV).", a_sum, b_sum);	// for debugging only
	return(eps*U_sum);
}