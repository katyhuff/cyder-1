/*! \file OneDimPPMNuclide.cpp
    \brief Implements the OneDimPPMNuclide class used by the Generic Repository 
    \author Kathryn D. Huff
 */
#include <iostream>
#include <fstream>
#include <deque>
#include <vector>
#include <time.h>
#include <boost/lexical_cast.hpp>
#include <boost/math/special_functions/erf.hpp>
#include <boost/numeric/odeint.hpp>


#include "CycException.h"
#include "Logger.h"
#include "Timer.h"
#include "OneDimPPMNuclide.h"

using namespace std;
using boost::lexical_cast;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OneDimPPMNuclide::OneDimPPMNuclide():
  v_(0),
  porosity_(0),
  rho_(0)
{
  Co_ = IsoConcMap();
  Ci_ = IsoConcMap();
  set_geom(GeometryPtr(new Geometry()));
  last_updated_=0;

  wastes_ = deque<mat_rsrc_ptr>();
  vec_hist_ = VecHist();
  conc_hist_ = ConcHist();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OneDimPPMNuclide::OneDimPPMNuclide(QueryEngine* qe):
  v_(0),
  porosity_(0),
  rho_(0)
{
  Co_ = IsoConcMap();
  Ci_ = IsoConcMap();
  wastes_ = deque<mat_rsrc_ptr>();
  set_geom(GeometryPtr(new Geometry()));
  last_updated_=0;
  vec_hist_ = VecHist();
  conc_hist_ = ConcHist();
  initModuleMembers(qe);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OneDimPPMNuclide::~OneDimPPMNuclide(){ }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OneDimPPMNuclide::initModuleMembers(QueryEngine* qe){
  // advective velocity (hopefully the same as the whole system).
  v_ = lexical_cast<double>(qe->getElementContent("advective_velocity"));
  // rock parameters
  porosity_ = lexical_cast<double>(qe->getElementContent("porosity"));
  rho_ = lexical_cast<double>(qe->getElementContent("bulk_density"));

  LOG(LEV_DEBUG2,"GR1DNuc") << "The OneDimPPMNuclide Class init(cur) function has been called";;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NuclideModelPtr OneDimPPMNuclide::copy(const NuclideModel& src){
  const OneDimPPMNuclide* src_ptr = dynamic_cast<const OneDimPPMNuclide*>(&src);

  set_last_updated(TI->time());
  set_porosity(src_ptr->porosity());
  set_rho(src_ptr->rho());
  set_v(src_ptr->v());


  // copy the geometry AND the centroid. It should be reset later.
  set_geom(geom_->copy(src_ptr->geom(), src_ptr->geom()->centroid()));

  wastes_ = deque<mat_rsrc_ptr>();
  vec_hist_ = VecHist();
  conc_hist_ = ConcHist();
  update_vec_hist(TI->time());

  return shared_from_this();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void OneDimPPMNuclide::updateNuclideParamsTable(){
  shared_from_this()->addRowToNuclideParamsTable("porosity", porosity());
  shared_from_this()->addRowToNuclideParamsTable("bulk_density", rho());
  shared_from_this()->addRowToNuclideParamsTable("advective_velocity", v());
  shared_from_this()->addRowToNuclideParamsTable("ref_disp", mat_table_->ref_disp());
  shared_from_this()->addRowToNuclideParamsTable("ref_kd", mat_table_->ref_kd());
  shared_from_this()->addRowToNuclideParamsTable("ref_sol", mat_table_->ref_sol());
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void OneDimPPMNuclide::print(){
    LOG(LEV_DEBUG2,"GR1DNuc") << "OneDimPPMNuclide Model";;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void OneDimPPMNuclide::absorb(mat_rsrc_ptr matToAdd)
{
  // Get the given OneDimPPMNuclide's contaminant material.
  // add the material to it with the material absorb function.
  // each nuclide model should override this function
  LOG(LEV_DEBUG2,"GR1DNuc") << "OneDimPPMNuclide is absorbing material: ";
  matToAdd->print();
  wastes_.push_back(matToAdd);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
mat_rsrc_ptr OneDimPPMNuclide::extract(const CompMapPtr comp_to_rem, double kg_to_rem)
{
  // Get the given OneDimPPMNuclide's contaminant material.
  // add the material to it with the material extract function.
  // each nuclide model should override this function
  LOG(LEV_DEBUG2,"GR1DNuc") << "OneDimPPMNuclide" << "is extracting composition: ";
  comp_to_rem->print() ;
  mat_rsrc_ptr to_ret = mat_rsrc_ptr(MatTools::extract(comp_to_rem, kg_to_rem, wastes_));
  update(last_updated());
  return to_ret;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void OneDimPPMNuclide::transportNuclides(int the_time){
  // This should transport the nuclides through the component.
  // It will likely rely on the internal flux and will produce an external flux. 
  // The convention will be that flux is positive in the radial direction
  // If these fluxes are negative, nuclides aphysically flow toward the waste package 
  // It will send the adjacent components information?
  // The OneDimPPMNuclide class should transport all nuclides
  update(the_time);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void OneDimPPMNuclide::update(int the_time){
  update_vec_hist(the_time);
  update_conc_hist(the_time, wastes_);
  set_last_updated(the_time);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
pair<IsoVector, double> OneDimPPMNuclide::source_term_bc(){
  double tot_mass=0;
  IsoConcMap conc_map = MatTools::scaleConcMap(conc_hist(last_updated()), V_f());
  CompMapPtr comp_map = CompMapPtr(new CompMap(MASS));
  IsoConcMap::iterator it;
  for( it=conc_map.begin(); it!=conc_map.end(); ++it){
    (*comp_map)[(*it).first]=(*it).second;
    tot_mass += (*it).second;
  }
  IsoVector to_ret = IsoVector(comp_map);
  return make_pair(to_ret, tot_mass);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
IsoConcMap OneDimPPMNuclide::dirichlet_bc(){
  return conc_hist(last_updated());
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
ConcGradMap OneDimPPMNuclide::neumann_bc(IsoConcMap c_ext, Radius r_ext){
  ConcGradMap to_ret;
  IsoConcMap c_int = conc_hist(last_updated());
  Radius r_int = geom()->radial_midpoint();

  int iso;
  IsoConcMap::iterator it;
  for( it=c_int.begin(); it!=c_int.end(); ++it ){
    iso=(*it).first;
    if(c_ext.count(iso) != 0 ){
      // in both
      to_ret[iso] = calc_conc_grad(c_ext[iso], c_int[iso], r_ext, r_int);
    } else { 
      // in c_int only
      to_ret[iso] = calc_conc_grad(0, c_int[iso], r_ext, r_int);
    }
  }
  for( it=c_ext.begin(); it!=c_ext.end(); ++it){
    iso = (*it).first;
    if( c_int.count(iso) == 0 ){
      // in c_ext only
      to_ret[iso] = calc_conc_grad(c_ext[iso], 0, r_ext, r_int);
    }
  }
  return to_ret;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
IsoFluxMap OneDimPPMNuclide::cauchy_bc(IsoConcMap c_ext, Radius r_ext){
  IsoFluxMap to_ret;
  ConcGradMap neumann = neumann_bc(c_ext,r_ext);

  ConcGradMap::iterator it;
  Iso iso;
  Elem elem;
  for( it=neumann.begin(); it!=neumann.end(); ++it){
    iso=(*it).first;
    elem=iso/1000;
    to_ret.insert(make_pair(iso, -mat_table_->D(elem)*(*it).second + shared_from_this()->dirichlet_bc(iso)*v()));
  }

  return to_ret;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void OneDimPPMNuclide::update_vec_hist(int the_time){
  vec_hist_[the_time]=MatTools::sum_mats(wastes_);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void OneDimPPMNuclide::update_conc_hist(int the_time){
  return update_conc_hist(the_time, wastes_);
}
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void OneDimPPMNuclide::update_conc_hist(int the_time, deque<mat_rsrc_ptr> mats){
  assert(last_updated() <= the_time);
  IsoConcMap to_ret;

  pair<IsoVector, double> sum_pair = MatTools::sum_mats(mats);
  IsoConcMap C_0 = MatTools::comp_to_conc_map(sum_pair.first.comp(), sum_pair.second, V_f());

  Radius r_calc = geom_->radial_midpoint();
  // @TODO funky?
  to_ret = conc_profile(C_0, r_calc, the_time);
  set_last_updated(the_time);
  conc_hist_[the_time] = to_ret;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
IsoConcMap OneDimPPMNuclide::conc_profile(IsoConcMap C_0, Radius r, int dt){
  // @TODO decay will accidentally get neglected here if you don't watch out. Fix.
  IsoConcMap to_ret;
  IsoConcMap::iterator it;
  Iso iso;
  for(it=C_0.begin(); it!=C_0.end(); ++it){
    iso = (*it).first;
    // @TODO FIX C_0 to C_i
    to_ret[iso] = calculate_conc(C_0, C_0, r, iso, dt, dt+dt);
  }
  return to_ret;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
IsoConcMap OneDimPPMNuclide::calculate_conc(IsoConcMap C_0, IsoConcMap C_i, double r, int t0, int t) {
  IsoConcMap::iterator it;
  int iso;
  IsoConcMap to_ret;
  for(it=C_0.begin(); it!=C_0.end(); ++it){
    iso=(*it).first;
    to_ret[iso] = calculate_conc(C_0, C_i, r, iso, t0, t);
  }
  return to_ret;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
double OneDimPPMNuclide::calculate_conc(IsoConcMap C_0, IsoConcMap C_i, double r, Iso iso, int t0, int t) {
  double D = mat_table_->D(iso/1000);
  double pi = boost::math::constants::pi<double>();
  //@TODO add sorption to this model. For now, R=1, no sorption. 
  double R=1;
  //@TODO convert months to seconds

  double At_frac = (R*r - v()*t)/(2*pow(D*R*t, 0.5));
  double At = 0.5*boost::math::erfc(At_frac);
  double Att0_frac = (R*r - v()*t)/(2*pow(D*R*(t-t0), 0.5));
  double Att0 = 0.5*boost::math::erfc(Att0_frac);
  double A = At - Att0;
  double B1_frac = (R*r - v()*t)/(2*pow(D*R*t, 0.5));
  double B1 = 0.5*boost::math::erfc(B1_frac);
  double B2_exp = -pow(R*r-v()*t,2)/(4*D*R*t);
  double B2 = pow(v()*v()*t/(pi*R*D),0.5)*exp(B2_exp);
  double B3_exp = v()*r/D;
  double B3_frac = (R*r + v()*t)/(2*pow(D*R*t,0.5));
  double B3_factor = -0.5*(1+(v()*r)/(D) + (v()*v()*t)/(D*R));
  double B3 = B3_factor*exp(B3_exp)*boost::math::erfc(B3_frac);  ;
  double B = C_i[iso]*(B1 + B2 + B3);

  double to_ret = C_0[iso]*A + B;
  
  return to_ret;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void OneDimPPMNuclide::update_inner_bc(int the_time, std::vector<NuclideModelPtr> daughters){
  std::vector<NuclideModelPtr>::iterator daughter;
  std::vector<IsoConcMap> c_t_n;
  int n=10;
  double a=geom()->inner_radius();
  double b=geom()->outer_radius();

  std::map<double, IsoConcMap> fmap;
  vector<double> calc_points = MatTools::linspace(a,b,n);
  vector<double>::iterator pt;
  for(daughter=daughters.begin(); daughter!=daughters.end(); ++daughter){
    // m(tn-1) = current contaminants
    std::pair<IsoVector, double> m_prev = MatTools::sum_mats(wastes_);
    // Ci = C(tn-1)
    for(pt = calc_points.begin(); pt!=calc_points.end(); ++pt){ 
      // C(tn) = f(Co_, Ci_)
      fmap[(*pt)] = calculate_conc(Co(), Ci(), (*pt), the_time-1, the_time); 
    }
    // m(tn) = integrate C_t_n
    IsoConcMap to_ret = trap_rule(a, b, n, fmap);
    pair<IsoVector, double> comp_t_n = MatTools::conc_to_comp_map(to_ret, geom()->volume());

    pair<IsoVector, double> m_ij = MatTools::subtractCompMaps(comp_t_n, m_prev);

    absorb(mat_rsrc_ptr((*daughter)->extract(CompMapPtr(m_ij.first.comp()),m_ij.second)));
  }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
IsoConcMap OneDimPPMNuclide::trap_rule(double a, double b, int n, map<double, IsoConcMap> fmap) {
  double scalar = (b-a)/(2*n);

  vector<IsoConcMap> terms;
  IsoConcMap f_0;
  IsoConcMap f_n;

  map<double, IsoConcMap>::iterator it = fmap.find(a);
  if(it!=fmap.end()){ 
    f_0 = fmap[a];
    fmap.erase(it);
  }
  it = fmap.find(b);
  if(it!=fmap.end()){ 
    f_n = fmap[b];
    fmap.erase(it);
  }
  terms.push_back(f_0);
  terms.push_back(f_n);
  map<double,IsoConcMap>::iterator f;
  for(f=fmap.begin(); f!=fmap.end(); ++f){
    terms[(*f).first] = MatTools::scaleConcMap((*f).second,2*scalar);
  }
  vector<IsoConcMap>::iterator t;
  IsoConcMap to_ret =IsoConcMap();
  IsoConcMap prev = IsoConcMap();
  for(t=terms.begin(); t!=terms.end(); ++t){
    to_ret = MatTools::addConcMaps(prev, (*t));
    prev = to_ret;
  }
  return to_ret;
}
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void OneDimPPMNuclide::set_porosity(double porosity){
  try { 
    MatTools::validate_percent(porosity);
  } catch (CycRangeException& e) {
    stringstream msg_ss;
    msg_ss << "The OneDimPPMNuclide porosity range is 0 to 1, inclusive.";
    msg_ss << " The value provided was ";
    msg_ss << porosity;
    msg_ss << ".";
    LOG(LEV_ERROR, "GRDRNuc") << msg_ss.str();;
    throw CycRangeException(msg_ss.str());
  }
  porosity_ = porosity;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void OneDimPPMNuclide::set_rho(double rho){
  try { 
    MatTools::validate_finite_pos(rho);
  } catch (CycRangeException& e) {
    stringstream msg_ss;
    msg_ss << "The OneDimPPMNuclide bulk density (rho) range must be positive and finite";
    msg_ss << " The value provided was ";
    msg_ss << rho;
    msg_ss << ".";
    LOG(LEV_ERROR, "GRDRNuc") << msg_ss.str();;
    throw CycRangeException(msg_ss.str());
  }
  rho_ = rho;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void OneDimPPMNuclide::set_Co(IsoConcMap Co){
  Co_=Co;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
const IsoConcMap OneDimPPMNuclide::Ci() const {
  pair<IsoVector, double> st = MatTools::sum_mats(wastes_);
  MatTools::comp_to_conc_map(CompMapPtr(st.first.comp()), st.second, geom()->volume());
}
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void OneDimPPMNuclide::set_Ci(IsoConcMap Ci){
  Ci_ =  Ci;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void OneDimPPMNuclide::set_v(double v){
  v_=v;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
double OneDimPPMNuclide::V_T(){
  return geom_->volume();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
double OneDimPPMNuclide::V_f(){
  return MatTools::V_f(V_T(), porosity());
}
