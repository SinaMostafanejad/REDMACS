#include <armadillo>
#include <iostream>
#include <fstream>

#include "mcpdft.h"

namespace mcpdft {

   MCPDFT::MCPDFT(std::string test_case)  { common_init(test_case); }
   MCPDFT::MCPDFT() {}
   MCPDFT::~MCPDFT() {}

   void MCPDFT::common_init(std::string test_case) {
       read_grids_from_file(test_case);
       read_orbitals_from_file(test_case);
       read_energies_from_file(test_case); 
       read_opdm_from_file(test_case);
       // read_cmat_from_file();
   }

   void MCPDFT::build_rho() {
      int nbfs = get_nbfs();
      size_t npts = get_npts();

      arma::mat phi(get_phi());
      arma::mat D1a(get_D1a());
      arma::mat D1b(get_D1b());
      arma::vec W(get_w());
      
      arma::vec rhoa(npts, arma::fill::zeros);
      arma::vec rhob(npts, arma::fill::zeros);
      arma::vec rho(npts, arma::fill::zeros);

      double dum_a = 0.0;
      double dum_b = 0.0;
      double dum_tot = 0.0;
      for(int p = 0; p < npts; p++) {
         double tempa = 0.0;
         double tempb = 0.0;
         for(int mu = 0; mu < nbfs; mu++) {
            for(int nu = 0; nu < nbfs; nu++) {
               tempa += D1a(mu, nu) * phi(p, mu) * phi(p, nu);
               tempb += D1b(mu, nu) * phi(p, mu) * phi(p, nu);
	    }
	 }
	 rhoa(p) = tempa;
	 rhob(p) = tempb;
	 rho(p) = rhoa(p) + rhob(p);

         dum_a += rhoa(p) * W(p);
         dum_b += rhob(p) * W(p);
         dum_tot += ( rhoa(p) + rhob(p) ) * W(p) ;
      }
      set_rhoa(rhoa);
      set_rhob(rhob);
      set_rho(rho);

      printf("\n");
      printf("  Integrated total density = %20.12lf\n",dum_tot);
      printf("  Integrated alpha density = %20.12lf\n",dum_a);
      printf("  Integrated beta density  = %20.12lf\n",dum_b);
      printf("\n");
   }

   void MCPDFT::build_pi(const arma::mat &D2ab) {
        int nbfs = get_nbfs();
        size_t npts = get_npts();

        arma::vec temp(npts);
        arma::mat phi(get_phi());

        for (int p = 0; p < npts; p++) {
                double dum = 0.0;
                // pi(r,r) = D(mu,nu; lambda,sigma) * phi(r,mu) * phi(r,nu) * phi(r,lambda) * phi(r,sigma)
                for (int mu = 0; mu < nbfs; mu++) {
                    for (int nu = 0; nu < nbfs; nu++) {
                        for (int lambda = 0; lambda < nbfs; lambda++) {
                            for (int sigma = 0; sigma < nbfs; sigma++) {
                                dum += phi(p, mu) * phi(p, nu) * phi(p, lambda) * phi(p, sigma) * D2ab(nu*nbfs+mu, sigma*nbfs+lambda);
                            }
                        }
                    }
                }
                temp(p) = dum;
        }
        set_pi(temp);
   }

   void MCPDFT::build_R() {
        double tol = 1.0e-20;
        size_t npts = get_npts();

        arma::vec temp(npts);
        arma::vec pi(get_pi());
        arma::vec rho(get_rho());

        for (int p = 0; p < npts; p++) {
            temp(p) = 4.0 * pi(p) / ( rho(p) * rho(p) );
        }
        set_R(temp);
   }

   void MCPDFT::translate() {
     double tol = 1.0e-20;
     size_t npts = get_npts();

     arma::vec rho_vec(get_rho());
     arma::vec pi_vec(get_pi());
     arma::vec R_vec(get_R());
     arma::vec W(get_w());
     arma::vec tr_rhoa(npts);
     arma::vec tr_rhob(npts);

     double dum_a = 0.0;
     double dum_b = 0.0;
     double dum_tot = 0.0;
     for (int p = 0; p < npts; p++) {

         double rho = rho_vec(p);
         double pi = pi_vec(p);
         double zeta = 0.0;
         double R = 0.0;
         if ( !(rho < tol) && !(pi < 0.0) ) {
            R = R_vec(p);
            if ( (1.0 - R) > tol ) {
               zeta = sqrt(1.0 - R);
            }else{
                 zeta = 0.0;
            }
            tr_rhoa(p) = (1.0 + zeta) * (rho/2.0);
            tr_rhob(p) = (1.0 - zeta) * (rho/2.0);
         }else {
                tr_rhoa(p) = 0.0;
                tr_rhob(p) = 0.0;
         }
         dum_a += tr_rhoa(p) * W(p);
         dum_b += tr_rhob(p) * W(p);
         dum_tot += ( tr_rhoa(p) + tr_rhob(p) ) * W(p) ;
     }
     set_tr_rhoa(tr_rhoa);
     set_tr_rhob(tr_rhob);

     printf("\n");
     printf("  Integrated translated total density = %20.12lf\n",dum_tot);
     printf("  Integrated translated alpha density = %20.12lf\n",dum_a);
     printf("  Integrated translated beta density  = %20.12lf\n",dum_b);
     printf("\n");
   }
 
   void MCPDFT::fully_translate(){
        double tol = 1.0e-20;
        size_t npts = get_npts();

        arma::vec rho_vec(get_rho());
        arma::vec pi_vec(get_pi());
        arma::vec R_vec(get_R());
        arma::vec W(get_w());

        arma::vec tr_rhoa(npts);
        arma::vec tr_rhob(npts);
        arma::vec tr_rho(npts);

        double const R0 = 0.9;
        double const R1 = 1.15;
        double const A = -475.60656009;
        double const B = -379.47331922;
        double const C = -85.38149682;

        double temp_tot = 0.0;
        double temp_a = 0.0;
        double temp_b = 0.0;
        for (int p = 0; p < npts; p++) {

            double zeta = 0.0;
            double R = 0.0;
            double rho = rho_vec(p);
            double pi = pi_vec(p);
            double DelR = R_vec(p) - R1;

            if ( !(rho < tol) && !(pi < 0.0) ) {

               R = R_vec(p);

               if ( ((1.0 - R) > tol) && ( R < R0 ) ) {

                  zeta = sqrt(1.0 - R);

               }else if( !(R < R0) && !(R > R1) ) {

                       zeta = A * pow(DelR, 5.0) + B * pow(DelR, 4.0) + C * pow(DelR, 3.0);

               }else if( R > R1 ) {

                       zeta = 0.0;
               }

               tr_rhoa(p) = (1.0 + zeta) * (rho/2.0);
               tr_rhob(p) = (1.0 - zeta) * (rho/2.0);

            }else{

                 tr_rhoa(p) = 0.0;
                 tr_rhob(p) = 0.0;
            }

            temp_a += tr_rhoa(p) * W(p);
            temp_b += tr_rhob(p) * W(p);
            temp_tot += ( tr_rhob(p) + tr_rhoa(p) ) * W(p);

        }
        set_tr_rhoa(tr_rhoa);
        set_tr_rhob(tr_rhob);

        printf("\n");
        printf("      Integrated fully translated total density = %20.12lf\n",temp_tot);
        printf("      Integrated fully translated alpha density = %20.12lf\n",temp_a);
        printf("      Integrated fully translated beta density  = %20.12lf\n",temp_b);
        printf("\n");

        //if ( is_gga_ || is_meta_ ) {
        // arma::vec tr_rho_a_x(ngrids_);
        // arma::vec tr_rho_b_x(ngrids_);

        // arma::vec tr_rho_a_y(ngrids_);
        // arma::vec tr_rho_b_y(ngrids_);

        // arma::vec tr_rho_a_z(ngrids_);
        // arma::vec tr_rho_b_z(ngrids_);

        // arma::vec tr_sigma_aa(ngrids_);
        // arma::vec tr_sigma_bb(ngrids_);
        // arma::vec tr_sigma_ab(ngrids_);

        // arma::mat temp_tr_rho1_a(ngrids_,3);
        // arma::mat temp_tr_rho1_b(ngrids_,3);
        // for (int p = 0; p < ngrids_; p++) {

        //     double rho_x = rho_a_x_(p) + rho_b_x_(p);
        //     double rho_y = rho_a_y_(p) + rho_b_y_(p);
        //     double rho_z = rho_a_z_(p) + rho_b_z_(p);

        //     double rho = rho_p(p);
        //     double pi = pi_p(p);
        //     double DelR = R_p(p) - R1;

        //     double zeta = 0.0;
        //     double R = 0.0;

        //     // if ( !(rho < tol) && !(pi < tol) ) {
        //     if ( !(rho < tol) && !(pi < 0.0) ) {

        //         R = R_p(p);
        //         // R = tanh(R);

        //         if ( ((1.0 - R) > tol) && ( R < R0 ) ) {

        //            zeta = sqrt(1.0 - R);

        //            tr_rho_a_x(p) = (1.0 + zeta) * (rho_x/2.0) + (R * rho_x) / (2.0*zeta) - pi_x_(p) / (rho*zeta);
        //            tr_rho_b_x(p) = (1.0 - zeta) * (rho_x/2.0) - (R * rho_x) / (2.0*zeta) + pi_x_(p) / (rho*zeta);

        //            tr_rho_a_y(p) = (1.0 + zeta) * (rho_y/2.0) + (R * rho_y) / (2.0*zeta) - pi_y_(p) / (rho*zeta);
        //            tr_rho_b_y(p) = (1.0 - zeta) * (rho_y/2.0) - (R * rho_y) / (2.0*zeta) + pi_y_(p) / (rho*zeta);

        //            tr_rho_a_z(p) = (1.0 + zeta) * (rho_z/2.0) + (R * rho_z) / (2.0*zeta) - pi_z_(p) / (rho*zeta);
        //            tr_rho_b_z(p) = (1.0 - zeta) * (rho_z/2.0) - (R * rho_z) / (2.0*zeta) + pi_z_(p) / (rho*zeta);

        //         }else if( !(R < R0) && !(R > R1) ) {

        //                 zeta = A * pow(DelR, 5.0) + B * pow(DelR, 4.0) + C * pow(DelR, 3.0);

        //                 tr_rho_a_x(p) = (1.0 + zeta) * (rho_x/2.0)
        //                                + (A * pow(DelR, 4.0)) * ( (10.0 * pi_x_(p) / rho) - (5.0 * R * rho_x) )
        //                                + (B * pow(DelR, 3.0)) * ( (8.0  * pi_x_(p) / rho) - (4.0 * R * rho_x) )
        //                                + (C * pow(DelR, 2.0)) * ( (6.0  * pi_x_(p) / rho) - (3.0 * R * rho_x) );

        //                 tr_rho_b_x(p) = (1.0 - zeta) * (rho_x/2.0)
        //                                + (A * pow(DelR, 4.0)) * (-(10.0 * pi_x_(p) / rho) + (5.0 * R * rho_x) )
        //                                + (B * pow(DelR, 3.0)) * (-(8.0  * pi_x_(p) / rho) + (4.0 * R * rho_x) )
        //                                + (C * pow(DelR, 2.0)) * (-(6.0  * pi_x_(p) / rho) + (3.0 * R * rho_x) );

        //                 tr_rho_a_y(p) = (1.0 + zeta) * (rho_y/2.0)
        //                                + (A * pow(DelR, 4.0)) * ( (10.0 * pi_y_(p) / rho) - (5.0 * R * rho_y) )
        //                                + (B * pow(DelR, 3.0)) * ( (8.0  * pi_y_(p) / rho) - (4.0 * R * rho_y) )
        //                                + (C * pow(DelR, 2.0)) * ( (6.0  * pi_y_(p) / rho) - (3.0 * R * rho_y) );

        //                 tr_rho_b_y(p) = (1.0 - zeta) * (rho_y/2.0)
        //                                + (A * pow(DelR, 4.0)) * (-(10.0 * pi_y_(p) / rho) + (5.0 * R * rho_y) )
        //                                + (B * pow(DelR, 3.0)) * (-(8.0  * pi_y_(p) / rho) + (4.0 * R * rho_y) )
        //                                + (C * pow(DelR, 2.0)) * (-(6.0  * pi_y_(p) / rho) + (3.0 * R * rho_y) );

        //                 tr_rho_a_z(p) = (1.0 + zeta) * (rho_z/2.0)
        //                                + (A * pow(DelR, 4.0)) * ( (10.0 * pi_z_(p) / rho) - (5.0 * R * rho_z) )
        //                                + (B * pow(DelR, 3.0)) * ( (8.0  * pi_z_(p) / rho) - (4.0 * R * rho_z) )
        //                                + (C * pow(DelR, 2.0)) * ( (6.0  * pi_z_(p) / rho) - (3.0 * R * rho_z) );

        //                 tr_rho_b_z(p) = (1.0 - zeta) * (rho_z/2.0)
        //                                 + (A * pow(DelR, 4.0)) * (-(10.0 * pi_z_(p) / rho) + (5.0 * R * rho_z) )
        //                                 + (B * pow(DelR, 3.0)) * (-(8.0  * pi_z_(p) / rho) + (4.0 * R * rho_z) )
        //                                 + (C * pow(DelR, 2.0)) * (-(6.0  * pi_z_(p) / rho) + (3.0 * R * rho_z) );
        //         }else if( R > R1 ) {
        //                 zeta = 0.0;

        //                 tr_rho_a_x(p) = (1.0 + zeta) * (rho_x/2.0);
        //                 tr_rho_b_x(p) = (1.0 - zeta) * (rho_x/2.0);

        //                 tr_rho_a_y(p) = (1.0 + zeta) * (rho_y/2.0);
        //                 tr_rho_b_y(p) = (1.0 - zeta) * (rho_y/2.0);

        //                 tr_rho_a_z(p) = (1.0 + zeta) * (rho_z/2.0);
        //                 tr_rho_b_z(p) = (1.0 - zeta) * (rho_z/2.0);
        //         }

        //     }else{

        //         tr_rho_a_x(p) = 0.0;
        //         tr_rho_b_x(p) = 0.0;

        //         tr_rho_a_y(p) = 0.0;
        //         tr_rho_b_y(p) = 0.0;

        //         tr_rho_a_z(p) = 0.0;
        //         tr_rho_b_z(p) = 0.0;
        //     }

        //     temp_tr_rho1_a(p,0) = tr_rho_a_x(p); 
        //     temp_tr_rho1_a(p,1) = tr_rho_a_y(p); 
        //     temp_tr_rho1_a(p,2) = tr_rho_a_z(p); 
        //     temp_tr_rho1_b(p,0) = tr_rho_b_x(p); 
        //     temp_tr_rho1_b(p,1) = tr_rho_b_y(p); 
        //     temp_tr_rho1_b(p,2) = tr_rho_b_z(p); 

        //     tr_sigma_aa(p) = (tr_rho_a_x(p) * tr_rho_a_x(p)) + (tr_rho_a_y(p) * tr_rho_a_y(p)) + (tr_rho_a_z(p) * tr_rho_a_z(p));
        //     tr_sigma_ab(p) = (tr_rho_a_x(p) * tr_rho_b_x(p)) + (tr_rho_a_y(p) * tr_rho_b_y(p)) + (tr_rho_a_z(p) * tr_rho_b_z(p));
        //     tr_sigma_bb(p) = (tr_rho_b_x(p) * tr_rho_b_x(p)) + (tr_rho_b_y(p) * tr_rho_b_y(p)) + (tr_rho_b_z(p) * tr_rho_b_z(p));

        //  //}

        // }
        // set_tr_rho1_a(temp_tr_rho1_a);
        // set_tr_rho1_b(temp_tr_rho1_b);
   }
    void MCPDFT::build_opdm() {
       // fetching the number of basis functions
       int nbfs;
       nbfs = get_nbfs();
 
       // getting the AO->MO transformation matrix C
       arma::mat ca(get_cmat());
       arma::mat cb(get_cmat());
 
       // building the 1-electron reduced density matrices (1RDMs)
       arma::mat D1a(nbfs, nbfs, arma::fill::zeros);
       arma::mat D1b(nbfs, nbfs, arma::fill::zeros);
       for (int mu = 0; mu < nbfs; mu++) { 
           for (int nu = 0; nu < nbfs; nu++) { 
               double duma = 0.0;
               double dumb = 0.0;
               for (int i = 0; i < nbfs/2; i++) { 
                    duma += ca(mu, i) * ca(nu, i);
                    dumb += cb(mu, i) * cb(nu, i);
               }
 	      D1a(mu, nu) = duma;
               D1b(mu, nu) = dumb;
 	  }
       }
       // D1a(0,0) = 1.0;
       // D1b(0,0) = 1.0;
       D1a.print("D1a = ");
       D1b.print("D1b = ");
       set_D1a(D1a);
       set_D1b(D1b);
    }
 
    void MCPDFT::build_tpdm() {
       // fetching the number of basis functions
       int nbfs  = get_nbfs();
       int nbfs2 = nbfs * nbfs;
 
       arma::mat D1a(get_D1a());
       arma::mat D1b(get_D1b());
 
       arma::mat D2ab(nbfs2, nbfs2, arma::fill::zeros);
       D2ab = arma::kron(D1a,D1b);
       // D2ab.print("D2ab = ");
       set_D2ab(D2ab);
    }
 
    size_t MCPDFT::get_npts() const { return npts_; }
    int    MCPDFT::get_nbfs() const { return nbfs_; }
    arma::vec MCPDFT::get_w() const { return w_; }
   arma::vec MCPDFT::get_x() const { return x_; }
   arma::vec MCPDFT::get_y() const { return y_; }
   arma::vec MCPDFT::get_z() const { return z_; }
   arma::mat MCPDFT::get_phi() const { return phi_; }
   double MCPDFT::get_eref() const { return eref_; }
   double MCPDFT::get_eclass()  const { return eclass_; }
   arma::mat MCPDFT::get_cmat() const { return cmat_; }
   arma::mat MCPDFT::get_D1a()  const { return D1a_ ; }
   arma::mat MCPDFT::get_D1b()  const { return D1b_ ; }
   arma::mat MCPDFT::get_D2ab()  const { return D2ab_ ; }
   arma::vec MCPDFT::get_rhoa() const { return rho_a_; }
   arma::vec MCPDFT::get_rhob() const { return rho_b_; }
   arma::vec MCPDFT::get_rho() const { return rho_; }
   arma::vec MCPDFT::get_tr_rhoa() const { return tr_rho_a_; }
   arma::vec MCPDFT::get_tr_rhob() const { return tr_rho_b_; }
   arma::vec MCPDFT::get_tr_rho() const { return tr_rho_; }
   arma::vec MCPDFT::get_pi() const { return pi_; }
   arma::vec MCPDFT::get_R() const { return R_; }

   void MCPDFT::set_npts(const size_t npts) { npts_ = npts; }
   void MCPDFT::set_nbfs(const int nbfs)    { nbfs_ = nbfs; }
   void MCPDFT::set_w(const arma::vec &w) { w_ = w; }
   void MCPDFT::set_x(const arma::vec &x) { x_ = x; }
   void MCPDFT::set_y(const arma::vec &y) { y_ = y; }
   void MCPDFT::set_z(const arma::vec &z) { z_ = z; }
   void MCPDFT::set_phi(const arma::mat &phi) { phi_ = phi; }
   void MCPDFT::set_eref(const double eref) { eref_ = eref; }
   void MCPDFT::set_eclass(const double eclass) { eclass_ = eclass; }
   void MCPDFT::set_cmat(const arma::mat &cmat) { cmat_ = cmat; }
   void MCPDFT::set_D1a(const arma::mat &D1a) { D1a_ = D1a; }
   void MCPDFT::set_D1b(const arma::mat &D1b) { D1b_ = D1b; }
   void MCPDFT::set_D2ab(const arma::mat &D2ab) { D2ab_ = D2ab; }
   void MCPDFT::set_rhoa(const arma::vec &rhoa) { rho_a_ = rhoa; }
   void MCPDFT::set_rhob(const arma::vec &rhob) { rho_b_ = rhob; }
   void MCPDFT::set_rho(const arma::vec &rho) { rho_ = rho; }
   void MCPDFT::set_tr_rhoa(const arma::vec &tr_rhoa) { tr_rho_a_ = tr_rhoa; }
   void MCPDFT::set_tr_rhob(const arma::vec &tr_rhob) { tr_rho_b_ = tr_rhob; }
   void MCPDFT::set_tr_rho(const arma::vec &tr_rho) { tr_rho_ = tr_rho; }
   void MCPDFT::set_pi(const arma::vec &pi) { pi_ = pi; }
   void MCPDFT::set_R(const arma::vec &R) { R_ = R; }

}
