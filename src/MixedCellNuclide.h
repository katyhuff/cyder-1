// MixedCellNuclide.h
#if !defined(_MIXEDCELLNUCLIDE_H)
#define _MIXEDCELLNUCLIDE_H

#include <iostream>
#include "Logger.h"
#include <vector>
#include <map>
#include <string>

#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include "NuclideModel.h"


/** 
   @brief MixedCellNuclide is a nuclide transport model that treats the volume 
   as a homogeneously mixed cell.
   
   This disposal system nuclide model will receive material diffusively. That is, at
   the internal boundary, if the concentration from an adjacent cell is
   higher than the concentration within the mixed cell, this cell will accept the 
   appropriate flux accross that boundary. So too if the concentration is lower in the 
   adjacent cell, a corresponding contaminant flux will leave this cell accross that boundary. 
   
   When accepting a flux of Material, this cell immediately incorporates it into 
   the mixed volume. The mixed volume will then make the corresponding homogeneous 
   concentration available at all boundaries.  
   
   The MixedCellNuclide model can be used to represent nuclide models of the 
   disposal system such as the Waste Form, Waste Package, Buffer, Near Field,
   Far Field, and Envrionment.
 */
class MixedCellNuclide : public NuclideModel {
public:
  
  /**
     Default constructor for the mixed cell nuclide model class.
     Creates an empty mixed cell with contaminant concentration zero unless 
     an initial contaminant concentration is specified.
   */
  MixedCellNuclide(); 

  /**
     primary constructor reads input from XML node
     
     @param cur input XML node
   */
  MixedCellNuclide(xmlNodePtr cur){};

  /** 
     Default destructor does nothing.
   */
  ~MixedCellNuclide() {};

  /**
     initializes the model parameters from an xmlNodePtr
     
     @param cur is the current xmlNodePtr
   */
  virtual void init(xmlNodePtr cur); 

  /**
     copies a nuclide model and its parameters from another
     
     @param src is the nuclide model being copied
   */
  virtual NuclideModel* copy(NuclideModel* src); 

  /**
     standard verbose printer includes current temp and concentrations
   */
  virtual void print(); 

  /**
     Absorbs the contents of the given Material into this MixedCellNuclide.
     
     @param matToAdd the MixedCellNuclide to be absorbed
   */
  virtual void absorb(mat_rsrc_ptr matToAdd) ;

  /**
     Extracts the contents of the given Material from this MixedCellNuclide. Use this 
     function for decrementing a MixedCellNuclide's mass balance after transferring 
     through a link. 
     
     @param matToRem the Material whose composition we want to decrement 
     against this MixedCellNuclide
   */
  virtual void extract(mat_rsrc_ptr matToRem) ;

  /**
     Transports nuclides from the inner to the outer boundary 
   */
  virtual void transportNuclides();

  /**
     Returns the nuclide model type
   */
  virtual NuclideModelType getNuclideModelType(){return MIXEDCELL_NUCLIDE;};

  /**
     Returns the nuclide model type name
   */
  virtual std::string getNuclideModelName(){return "MIXEDCELL_NUCLIDE";};

  /**
     returns the peak Toxicity this object will experience during the 
     simulation.
     
     @return peak_toxicity
   */
  const virtual Tox getPeakTox(){};

  /**
     returns the concentration map for this component at the time specified
     
     @param time the time to query the concentration map
   */
  virtual ConcMap getConcMap(int time){};

private:
  /**
     mixes the cell's contents to populate the concentration map
   */
  void mixCell();

  /**
    a fraction (0-1) representing the porosity of the undegraded matrix.
   */
  double porosity_;

  /**
    a fraction (0-1) representing the degradation rate of this volume.
   */
  double deg_rate_;

};


#endif