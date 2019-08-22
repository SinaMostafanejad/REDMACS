#ifndef MCPDFT_H
#define MCPDFT_H

#include <armadillo>

namespace mcpdft{

class MCPDFT {

   public:

      /// constructor
      MCPDFT();
      /// destructor
      ~MCPDFT();

      /// initialize the class member variables
      void common_init();

      /// Computes the MCPDFT energy
      double mcpdft_energy(const arma::mat &D1a,
                           const arma::mat &D1b,
                           const arma::mat &D2ab);

      //=========== utility functions ============//
      void read_grids_from_file();
      void read_energies_from_file();
      void read_cmat_from_file();

      //=============== accessors ===============//

      size_t get_npts() const;
      arma::vec get_w() const;
      arma::vec get_x() const;
      arma::vec get_y() const;
      arma::vec get_z() const;
      arma::vec get_phi() const;
      double get_eref() const;
      double get_eclass() const;
      arma::mat get_cmat() const;

      void set_npts(const size_t npts);
      void set_w(const arma::vec &w);
      void set_x(const arma::vec &x);
      void set_y(const arma::vec &y);
      void set_z(const arma::vec &z);
      void set_phi(const arma::vec &phi);
      void set_eref(const double eref);
      void set_eclass(const double eclass);
      void set_cmat(const arma::mat &cmat);

   protected:

   private:

//       /// 1-electron reduced-density matrix of alpha spin
//       arma::mat D1a_;
// 
//       /// 1-electron reduced-density matrix of beta spin
//       arma::mat D1b_;
// 
//       /// 2-electron reduced-density matrix of alpha-beta spin
//       arma::mat D2ab_;

      /// number of grid points

      size_t npts_;
      
      /// vector of weights for quadrature grid points
      arma::vec w_;

      /// vector of x-coordinates for quadrature grid points
      arma::vec x_;

      /// vector of y-coordinates for quadrature grid points
      arma::vec y_;

      /// vector of z-coordinates for quadrature grid points
      arma::vec z_;

      /// vector of orbital values calculated on the grid points
      arma::vec phi_;

      /// the reference electronic energy (read from file)
      double eref_;

      /// the classical electronic energy (read from file)
      double eclass_;

      /// AO->MO transformation matrix C (read from file)
      arma::mat cmat_;
};

}
#endif // MCPDFT_H
