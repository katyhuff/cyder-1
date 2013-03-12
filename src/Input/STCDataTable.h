// STCDataTable.h
#if !defined(_STCDATATABLE)
#define _STCDATATABLE

#include <string>
#include <vector>
#include <map>

#include <boost/shared_ptr.hpp>

/**
   Defines the structure of data associated with this row location in the  
   database.
 */
typedef struct mat_t
{
  double alpha_th; /**< a double indicating the thermal diffusivity of the material [] >**/
  double k_th; /**< a double indicating the thermal conductivity of the material [] >**/
  double spacing; /**< a double indicating the uniform spacing between packages [m] >**/
  double  r_calc; /**< a double indicating the radius where the STC is calculated >**/
  //double  <++>; /**< a double indicating the <++> of an element >**/
} mat_t;

/**
   Defines the structure of data associated with this row location in the  
   database.
 */
typedef struct stc_t
{
  double  iso; /**< a double indicating the <++> of an element >**/
  double  stc; /**< a double indicating the STC >**/
} element_t;

class STCDataTable;
typedef boost::shared_ptr<STCDataTable> STCDataTablePtr;

/**
   @class STCDataTable 
   The STCDataTable class provides an interface to the mat_data.sqlite 
   database, providing a robust and correct mass lookup by isotope 
 */
class STCDataTable {
private:

public:
  /**
     Default constructor for the STCDataTable class. 
     Default values are zeros and null strings
   */
  STCDataTable();

  /**
     Detailed constructor for the STCDataTable class
     Fully initializes the object

    @param mat the mat_ data member, a string
    @param elem_vec the elem_vec_ data member, a vector of element structs, the data
    @param elem_index the elem_index_ data member, mapping the element IDs to indices
    */
  STCDataTable(std::string mat, std::vector<element_t> elem_vec, std::map<Elem, int> elem_index);

  /**
     Destructor for the NullFacility class. 
     Makes certain to delete all appropriate data on the stack. 
   */
  ~STCDataTable();

  /**
     get the distribution coefficient [kg/kg] for some element in this material
      
     @param ent an identifier of type Elem, which is an int 

     @return K_d a double, the distribution coefficient [kg/kg] for the 
     element ent in the material mat. 
    */
  double K_d(Elem ent);

  /**
     get the solubility limit for some element in this material 
      
     @param ent an identifier of type Elem, which is an int 

     @return S a double, the solubility limit [kg/m^3] for the element 
     ent in the material mat. 
    */
  double S(Elem ent);

  /**
     get the dispersion coefficient [kg/m^2/s] for some element in this material
      
     @param ent an identifier of type Elem, which is an int 

     @return D a double, the dispersion coefficient [kg/m^2/s] for the 
     element ent in the material mat. 
    */
  double D(Elem ent);


  /** 
     gets a specific data object for some element in this material. 

     @param ent an identifier of type Elem, which is an int 
     @param data is a ChemDataType enum (DISP, KD, SOL, ...) 

     @return the data of type data for element elt in this material.
    */
  double data(Elem ent, ChemDataType data);


  /**
     returns the string name of the material that this table represents

     @return mat_
     */
  std::string mat(){return mat_;};

protected:
  /**
     checks whether this element has a row entry in the table.
     ideally all of them will... 
     @throws CycException when theres some drama
    */
  void check_validity(Elem ent);
  /**
     The name of the material that this table represents, 
     specifically, the name of the table in the DB
    */
  std::string name_;

  /** 
     The struct representing this material. 
     */
  mat_t_ mat_;

  /**
     The integer length (number of rows) of the tables 
   */
  int n_isos_;

  /**
     The vector of element structs that holds the data in the heat table 
   */
  std::vector<stc_t> stc_vec_;

  /** 
     a map for index lookup in the element vector. 
   */
  std::map<Iso, int> iso_index_;
};

#endif
