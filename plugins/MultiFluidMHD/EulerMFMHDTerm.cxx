#include "Config/BiggerThanZero.hh"
#include "MultiFluidMHD/EulerMFMHDTerm.hh"
#include "MathTools/MathConsts.hh"
#include "Framework/PhysicalConsts.hh"
#include "Framework/SubSystemStatus.hh"
#include "MultiFluidMHDModel.hh"

//////////////////////////////////////////////////////////////////////////////

using namespace std;
using namespace COOLFluiD::Config;
using namespace COOLFluiD::Framework;

//////////////////////////////////////////////////////////////////////////////

namespace COOLFluiD {

  namespace Physics {

    namespace MultiFluidMHD {

//////////////////////////////////////////////////////////////////////////////

void EulerMFMHDTerm::defineConfigOptions(Config::OptionList& options)
{
  options.addConfigOption< CFreal , ValidateOption < BiggerThanZero > > ("gamma","Specific heat ratio.");
  options.addConfigOption< CFreal > ("Omega", "RPM in revolutions/min");
  options.addConfigOption< CFreal > ("molecularMass1", "Molecular mass of first species");
  options.addConfigOption< CFreal > ("molecularMass2", "Molecular mass of second species");
  options.addConfigOption< CFreal > ("molecularMass3", "Molecular mass of third species");
  options.addConfigOption< std::vector<CFreal> >("nonInducedElectromagnetic", "nonInduced Electromagnetic Field");
  options.addConfigOption< CFreal > ("lightSpeedMF", "Speed of light. It can be reduced if it is still bigger than the speed of the fluid");
  options.addConfigOption< bool > ("IsLeake", "Flag to use  the Leake model in the convective term.");
  options.addConfigOption< std::vector<std::string> >("Vars","Definition of the Variables.");
  options.addConfigOption< std::vector<std::string> >("DefEMField","Definition of the Functions.");
}

//////////////////////////////////////////////////////////////////////////////

EulerMFMHDTerm::EulerMFMHDTerm(const std::string& name) : 
  Maxwell::MaxwellProjectionTerm(name),
  _useFunction(false),
  _NonInducedEMField()
{ 
  addConfigOptionsTo(this);
   
  _gamma = 5./3.;
  setParameter("gamma",&_gamma); 
  
  _omega = 0.;
  setParameter("Omega",&_omega);
  
  _K = Framework::PhysicalConsts::Boltzmann();			//Boltzman's constant [J/K]
  
  _epsilon = Framework::PhysicalConsts::VacuumPermittivity();   //Permittivity of free space [F/m]
  
  _mu = Framework::PhysicalConsts::VacuumPermeability();        /// Magnetic permeability in vacuum [Volt·s/(Ampere·m)]
  
  _lightSpeed = Framework::PhysicalConsts::LightSpeed();		//light Speed m/s
  setParameter("lightSpeedMF",&_lightSpeed);
  
  _molecularMass1 = Framework::PhysicalConsts::ElectronMass();		// Electron's mass [kg] source:Standart Handbook for Electrical Engineerings   
  setParameter("molecularMass1",&_molecularMass1);  
  
  _molecularMass2 = Framework::PhysicalConsts::HydrogenMass();		// Neutral's mass [kg] source:Standart Handbook for Electrical Engineerings 
  setParameter("molecularMass2",&_molecularMass2);  
  
  _molecularMass3 = Framework::PhysicalConsts::ProtonMass();		// Proton's mass [kg/mol] equal to proton's mass   
  setParameter("molecularMass3",&_molecularMass3);
  
  _nonInducedEMField = std::vector<CFreal>();
  setParameter("nonInducedElectromagnetic",&_nonInducedEMField);

  _isLeake = false;
  setParameter("IsLeake", &_isLeake);

  _functions = std::vector<std::string>();
  setParameter("DefEMField",&_functions);

  _vars = std::vector<std::string>();
  setParameter("Vars",&_vars);
}
      
//////////////////////////////////////////////////////////////////////////////

EulerMFMHDTerm::~EulerMFMHDTerm()
{
}

//////////////////////////////////////////////////////////////////////////////

void EulerMFMHDTerm::configure ( Config::ConfigArgs& args )
{
  Maxwell::MaxwellProjectionTerm::configure(args);
  
  _NonInducedEMField.resize(6);

  // conversion to radiants/sec
  _omega *= (2.*MathTools::MathConsts::CFrealPi()/60.);

}

//////////////////////////////////////////////////////////////////////////////

void EulerMFMHDTerm::resizePhysicalData(RealVector& physicalData)
{
  // resize the physical data
  cf_assert(getDataSize() > 0);
  physicalData.resize(getDataSize());

  //configuring the function with the electromagnetic field
  _variables.resize(Framework::PhysicalModelStack::getActive()->getDim() + 1);
  if(!_functions.empty())
  {
    _vFunction.setFunctions(_functions);
    _vFunction.setVariables(_vars);
    try {
      _vFunction.parse();
      _useFunction = true;
    }
    catch (Common::ParserException& e) {
      CFout << e.what() << "\n";
      throw; // retrow the exception to signal the error to the user
    }
  }
}  

//////////////////////////////////////////////////////////////////////////////

void EulerMFMHDTerm::setupPhysicalData()
{
  cf_assert(getDataSize() > 0);

  // set the size of each physical data in the StatesData
  resizePhysicalData(m_physicalData);
  resizePhysicalData(m_refPhysicalData);
}

//////////////////////////////////////////////////////////////////////////////

void EulerMFMHDTerm::computeNonInducedEMField(CFreal xCoord, CFreal yCoord, CFreal zCoord)
{
  if(!_useFunction){
    for (CFuint i = 0; i < 6; i++){
      _NonInducedEMField[i] = _nonInducedEMField[i];
    }
  }
  if(_useFunction){
    //const CFuint nbDim = PhysicalModelStack::getActive()->getDim();
    if(Framework::PhysicalModelStack::getActive()->getDim() == 2) //both 2D and 2.5D
    {
      _variables[0] = xCoord;
      _variables[1] = yCoord;
    }
    else // in 3D case
    {
      _variables[0] = xCoord;
      _variables[1] = yCoord;
      _variables[2] = zCoord; 
    } 
    
    _variables[Framework::PhysicalModelStack::getActive()->getDim()] = Framework::SubSystemStatusStack::getActive()->getCurrentTimeDim();
    _vFunction.evaluate(_variables, _NonInducedEMField);
  }
}

//////////////////////////////////////////////////////////////////////////////

} // namespace MultiFluidMHD

} // namespace Physics

} // namespace COOLFluiD

//////////////////////////////////////////////////////////////////////////////
