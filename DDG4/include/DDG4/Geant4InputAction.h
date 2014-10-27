// $Id: Geant4Converter.cpp 603 2013-06-13 21:15:14Z markus.frank $
//====================================================================
//  AIDA Detector description implementation for LCD
//--------------------------------------------------------------------
//
//====================================================================
#ifndef DD4HEP_DDG4_GEANT4INPUTACTION_H
#define DD4HEP_DDG4_GEANT4INPUTACTION_H

// Framework include files
#include "DDG4/Geant4Particle.h"
#include "DDG4/Geant4GeneratorAction.h"

// C/C++ include files
#include <vector>
#include <memory>

// Forward declarations
class G4Event;

/// Namespace for the AIDA detector description toolkit
namespace DD4hep  {

  /// Namespace for the Geant4 based simulation part of the AIDA detector description toolkit
  namespace Simulation  {

    /// Basic geant4 event reader class. This interface/base-class must be implemented by concrete readers.
    /** 
     * Base class to read input files containing simulation data.
     *
     *  \author  P.Kostka (main author)
     *  \author  M.Frank  (code reshuffeling into new DDG4 scheme)
     *  \version 1.0
     *  \ingroup DD4HEP_SIMULATION
     */
    class Geant4EventReader  {

    public:
      typedef Geant4Particle Particle;
      typedef std::vector<Particle*> Particles;
      /// Status codes of the event reader object. Anything with NOT low-bit set is an error.
      enum EventReaderStatus { 
	EVENT_READER_ERROR=0,
	EVENT_READER_OK=1,
	EVENT_READER_NO_DIRECT=2,
	EVENT_READER_NO_PRIMARIES=4,
	EVENT_READER_NO_FACTORY=6,
	EVENT_READER_IO_ERROR=8
      };
    protected:
      /// File name to be opened and read
      std::string m_name;
      /// Flag if direct event access is supported. To be explicitly set by subclass constructors
      bool m_directAccess;
      /// Current event number
      int  m_currEvent;
    public:
      /// Initializing constructor
      Geant4EventReader(const std::string& nam);
      /// Default destructor
      virtual ~Geant4EventReader();
      /// File name
      const std::string& name()  const   {  return m_name;   }
      /// Flag if direct event access (by event sequence number) is supported (Default: false)
      bool hasDirectAccess() const  {  return m_directAccess; }
      /// Move to the indicated event number.
      /** For pure sequential access, the default implementation 
       *  will skip events one by one.
       *  For technologies supporting direct event access the default
       *  implementation will be empty.
       *
       *  @return 
       */
      virtual EventReaderStatus moveToEvent(int event_number);
      /// Skip event. To be implemented for sequential sources
      virtual EventReaderStatus skipEvent();
      /// Read an event and fill a vector of MCParticles.
      /** The additional argument 
       */
      virtual EventReaderStatus readParticles(int event_number, Particles& particles) = 0;
    };

    /// Generic input action capable of using the Geant4EventReader class.
    /** 
     * Concrete implementation of the Geant4 generator action base class
     * populating Geant4 primaries from Geant4 and HepStd files.
     *
     *  \author  P.Kostka (main author)
     *  \author  M.Frank  (code reshuffeling into new DDG4 scheme)
     *  \version 1.0
     *  \ingroup DD4HEP_SIMULATION
     */
    class Geant4InputAction : public Geant4GeneratorAction {

    public:
      typedef Geant4Particle Particle;
      typedef std::vector<Particle*> Particles;
    protected:
      /// Property: input file
      std::string         m_input;
      /// Property: SYNCEVT
      int                 m_firstEvent;
      /// Property; interaction mask
      int                 m_mask;
      /// Property: Momentum downscaler for debugging
      double              m_momScale;
      /// Event reader object
      Geant4EventReader*  m_reader;

    public:
      /// Read an event and return a LCCollectionVec of MCParticles.
      int readParticles(int event_number, Particles& particles);
      /// helper to report Geant4 exceptions
      std::string issue(int i) const;

      /// Standard constructor
      Geant4InputAction(Geant4Context* context, const std::string& name);
      /// Default destructor
      virtual ~Geant4InputAction();
      /// Create particle vector
      Particles* new_particles() const { return new Particles; }
      /// Callback to generate primary particles
      virtual void operator()(G4Event* event);
    };
  }     /* End namespace Simulation   */
}       /* End namespace DD4hep */
#endif  /* DD4HEP_DDG4_GEANT4INPUTACTION_H  */