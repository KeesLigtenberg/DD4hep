//==========================================================================
//  AIDA Detector description implementation for LCD
//--------------------------------------------------------------------------
// Copyright (C) Organisation europeenne pour la Recherche nucleaire (CERN)
// All rights reserved.
//
// For the licensing terms see $DD4hepINSTALL/LICENSE.
// For the list of contributors see $DD4hepINSTALL/doc/CREDITS.
//
// Author     : M.Frank
//
//==========================================================================

// Framework include files
#include "DD4hep/LCDD.h"
#include "DD4hep/Printout.h"
#include "DD4hep/Factories.h"
#include "DD4hep/IDDescriptor.h"
#include "DD4hep/VolumeManager.h"
#include "DD4hep/DetectorTools.h"
#include "DD4hep/MatrixHelpers.h"
#include "DD4hep/objects/VolumeManagerInterna.h"

// C/C++ include files
#include <stdexcept>
#include <algorithm>

using namespace std;
using namespace DD4hep;
using namespace DD4hep::Geometry;

namespace  {
  /** @class VolIDTest
   *
   *  Test the volume manager by scanning the sensitive
   *  volumes of one or several subdetectors.
   *
   *  @author  M.Frank
   *  @version 1.0
   */
  struct VolIDTest  {
    typedef DetectorTools::PlacementPath Chain;
    typedef PlacedVolume::VolIDs         VolIDs;
    /// Helper to scan volume ids
    struct FND {
      const string& test;
      FND(const string& c) : test(c) {}
      bool operator()(const VolIDs::value_type& c) const { return c.first == test; }
    };
    IDDescriptor  m_iddesc;
    VolumeManager m_mgr;
    DetElement    m_det;

    /// Initializing constructor
    VolIDTest(LCDD& lcdd, DetElement sdet, size_t depth);
    /// Default destructor
    virtual ~VolIDTest() {}
    /// Check volume integrity
    void checkVolume(DetElement e, PlacedVolume pv, const VolIDs& child_ids, const Chain& chain)  const;
    /// Walk through tree of detector elements
    //void walk(DetElement de, VolIDs ids, const Chain& chain, size_t depth, size_t mx_depth)  const;
    /// Walk through tree of volume placements
    void walkVolume(DetElement e, PlacedVolume pv, VolIDs ids, const Chain& chain, size_t depth, size_t mx_depth)  const;

    /// Action routine to execute the test
    static long run(LCDD& lcdd,int argc,char** argv);
  };
}

/// Initializing constructor
VolIDTest::VolIDTest(LCDD& lcdd, DetElement sdet, size_t depth) : m_det(sdet) {
  m_mgr    = lcdd.volumeManager();
  if ( !m_det.isValid() )   {
    stringstream err;
    err << "The subdetector " << m_det.name() << " is not known to the geometry.";
    printout(INFO,"VolIDTest",err.str().c_str());
    throw runtime_error(err.str());
  }
  if ( !lcdd.sensitiveDetector(m_det.name()).isValid() )   {
    stringstream err;
    err << "The sensitive detector of subdetector " << m_det.name()
        << " is not known to the geometry.";
    printout(INFO,"VolIDTest",err.str().c_str());
    //throw runtime_error(err.str());
    return;
  }
  m_iddesc = lcdd.sensitiveDetector(m_det.name()).readout().idSpec();
  //walk(m_det,VolIDs(),Chain(),0,depth);
  PlacedVolume pv  = sdet.placement();
  VolIDs       ids = pv.volIDs();
  Chain        chain;
  chain.push_back(pv);
  checkVolume(sdet, pv, ids, chain);
  walkVolume(sdet, pv, ids, chain, 1, depth);
}

/// Check volume integrity
void VolIDTest::checkVolume(DetElement detector, PlacedVolume pv, const VolIDs& child_ids, const Chain& chain)  const {
  stringstream err, log;
  VolumeID     det_vol_id = detector.volumeID();
  VolumeID     vid = det_vol_id;
  DetElement   top_sdet, det_elem;
  VolumeManager::Context* mgr_ctxt = 0;
  
  try {
    vid       = m_iddesc.encode(child_ids);
    top_sdet  = m_mgr.lookupDetector(vid);
    det_elem  = m_mgr.lookupDetElement(vid);
    mgr_ctxt  = m_mgr.lookupContext(vid);
    
    if ( pv.volume().isSensitive() )  {
      PlacedVolume det_place = m_mgr.lookupPlacement(vid);
      if ( pv.ptr() != det_place.ptr() )   {
        err << "VolumeMgrTest: Wrong placement "
            << " got "        << det_place.name() << " (" << (void*)det_place.ptr() << ")"
            << " instead of " << pv.name()        << " (" << (void*)pv.ptr()        << ") "
            << " vid:" << volumeID(vid);
      }
      else if ( top_sdet.ptr() != detector.ptr() )   {
        err << "VolumeMgrTest: Wrong associated sub-detector element vid="  << volumeID(vid)
            << " got "        << top_sdet.path() << " (" << (void*)top_sdet.ptr() << ") "
            << " instead of " << detector.path() << " (" << (void*)detector.ptr() << ")"
            << " vid:" << volumeID(vid);
      }
      // This is sort of a bit wischi-waschi....
      else if ( !DetectorTools::isParentElement(detector,det_elem) )   {
        err << "VolumeMgrTest: Wrong associated detector element vid="  << volumeID(vid)
            << " got "        << det_elem.path() << " (" << (void*)det_elem.ptr() << ") "
            << " instead of " << detector.path() << " (" << (void*)detector.ptr() << ")"
            << " vid:" << volumeID(vid);
      }
      else if ( top_sdet.ptr() != m_det.ptr() )   {
        err << "VolumeMgrTest: Wrong associated detector "
            << " vid:" << volumeID(vid);
      }
    }
  }
  catch(const exception& ex) {
    err << "Lookup " << pv.name() << " id:" << volumeID(vid)
        << " path:" << detector.path() << " error:" << ex.what();
  }
  if ( pv.volume().isSensitive() || (0 != det_vol_id) )  {
    string id_desc;
    log << "Volume:"  << setw(50) << left << pv.name();
    if ( pv.volume().isSensitive() )  {
      IDDescriptor dsc = SensitiveDetector(pv.volume().sensitiveDetector()).readout().idSpec();
      log << " IDDesc:" << (char*)(dsc.ptr() == m_iddesc.ptr() ? "OK " : "BAD");
    }
    else  {
      log << setw(11) << " ";
    }
    id_desc = m_iddesc.str(vid);
    log << " [" << char(pv.volume().isSensitive() ? 'S' : 'N') << "] " << right
        << " vid:" << volumeID(vid)
        << " " << id_desc;
    if ( !err.str().empty() )   {
      printout(ERROR,m_det.name(),err.str()+" "+log.str());
      //throw runtime_error(err.str());
      return;
    }
    id_desc = m_iddesc.str(det_elem.volumeID());
    printout(INFO,m_det.name(),log.str());
    printout(INFO,m_det.name(),"  Elt:%-64s    vid:%s %s Parent-OK:%3s",
             det_elem.path().c_str(),volumeID(det_elem.volumeID()).c_str(),
             id_desc.c_str(),
             yes_no(DetectorTools::isParentElement(detector,det_elem)));

    try  {
      if ( pv.volume().isSensitive() )  {
        TGeoHMatrix trafo;
        for (size_t i = chain.size()-1; i > 0; --i)  {
          //for (size_t i = 0; i<chain.size(); ++i )  {
          const TGeoMatrix* mat = chain[i]->GetMatrix();
          trafo.MultiplyLeft(mat);
        }
        for (size_t i = chain.size(); i > 0; --i)  {
          const TGeoMatrix* mat = chain[i-1]->GetMatrix();
          ::printf("Placement [%d]  VolID:%s\t\t",int(i),chain[i-1].volIDs().str().c_str());
          mat->Print();
        }
        ::printf("Computed Trafo (from placements):\t\t");
        trafo.Print();
        det_elem  = m_mgr.lookupDetElement(vid);
        ::printf("DetElement Trafo: %s [%s]\t\t",
                 det_elem.path().c_str(),volumeID(det_elem.volumeID()).c_str());
        det_elem.nominal().worldTransformation().Print();
        ::printf("VolumeMgr  Trafo: %s [%s]\t\t",det_elem.path().c_str(),volumeID(vid).c_str());
        m_mgr.worldTransformation(vid).Print();
        if ( 0 == mgr_ctxt )  {
          printout(ERROR,m_det.name(),"VOLUME_MANAGER FAILED: Could not find entry for vid:%s.",
                   volumeID(vid).c_str());
        }
        if ( pv.ptr() == det_elem.placement().ptr() )   {
          // The computed transformation 'trafo' MUST be equal to:
          // m_mgr.worldTransformation(vid) AND det_elem.nominal().worldTransformation()
          int res1 = _matrixEqual(trafo, det_elem.nominal().worldTransformation());
          int res2 = _matrixEqual(trafo, m_mgr.worldTransformation(vid));
          if ( res1 != MATRICES_EQUAL || res2 != MATRICES_EQUAL )  {
            printout(ERROR,m_det.name(),"DETELEMENT_PLACEMENT FAILED: World transformation DIFFER.");
          }
          else  {
            printout(ERROR,m_det.name(),"DETELEMENT_PLACEMENT: PASSED. All matrices equal: %s",
                     volumeID(vid).c_str());
          }
        }
        else  {
          // The computed transformation 'trafo' MUST be equal to:
          // m_mgr.worldTransformation(vid)
          // The det_elem.nominal().worldTransformation() however is DIFFERENT!
          int res2 = _matrixEqual(trafo, m_mgr.worldTransformation(vid));
          if ( res2 != MATRICES_EQUAL )  {
            printout(ERROR,m_det.name(),"VOLUME_PLACEMENT FAILED: World transformation DIFFER.");
          }
          else  {
            printout(ERROR,m_det.name(),"VOLUME_PLACEMENT: PASSED. All matrices equal: %s",
                     volumeID(vid).c_str());
          }
        }
#if 0
        int ii=1;
        DetElement par = det_elem;
        while( (par.isValid()) )  {
          const TGeoMatrix* mat = par.placement()->GetMatrix();
          ::printf("Element placement [%d]  VolID:%s %s\t\t",int(ii),
                   par.placement().volIDs().str().c_str(), par.path().c_str());
          mat->Print();
          par = par.parent();
          ++ii;
        }
#endif
      }
    }
    catch(const exception& ex) {
      err << "Matrix " << pv.name() << " id:" << volumeID(vid)
          << " path:" << detector.path() << " error:" << ex.what();
    }
    
  }
}

/// Walk through tree of detector elements
void VolIDTest::walkVolume(DetElement detector, PlacedVolume pv, VolIDs ids, const Chain& chain,
                           size_t depth, size_t mx_depth)  const
{
  if ( depth <= mx_depth )  {
    const TGeoNode* current  = pv.ptr();
    TObjArray*  nodes        = current->GetNodes();
    int         num_children = nodes ? nodes->GetEntriesFast() : 0;

    for(int i=0; i<num_children; ++i)   {
      TGeoNode* node = (TGeoNode*)nodes->At(i);
      PlacedVolume place(node);
      VolIDs child_ids(ids);
      Chain  child_chain(chain);

      place.access(); // Test validity
      child_chain.push_back(place);
      child_ids.insert(child_ids.end(), place.volIDs().begin(), place.volIDs().end());
      //bool is_sensitive = place.volume().isSensitive();
      //if ( is_sensitive || !child_ids.empty() )  {
      checkVolume(detector, place, child_ids, child_chain);
      //}
      walkVolume(detector, place, child_ids, child_chain, depth+1, mx_depth);
    }
  }
}
#if 0
/// Walk through tree of volume placements
void VolIDTest::walk(DetElement detector, VolIDs ids, const Chain& chain, size_t depth, size_t mx_depth)  const   {
  if ( depth <= mx_depth )  {
    DetElement::Children::const_iterator i;
    PlacedVolume pv = detector.placement();
    VolIDs       child_ids(ids);
    Chain        child_chain(chain);
    VolumeID     det_vol_id   = detector.volumeID();
    //bool         is_sensitive = pv.volume().isSensitive() || (0 != det_vol_id);

    child_chain.push_back(pv);
    child_ids.insert(child_ids.end(), pv.volIDs().begin(), pv.volIDs().end());
    //if ( is_sensitive )  {
    checkVolume(detector, pv, child_ids, child_chain);
    //}
    walkVolume(detector, pv, child_ids, child_chain, depth+1, mx_depth);
  }
}
#endif
/// Action routine to execute the test
long VolIDTest::run(LCDD& lcdd,int argc,char** argv)    {
  printout(ALWAYS,"DD4hepVolumeMgrTest","++ Processing plugin...");
  for(int iarg=0; iarg<argc;++iarg)  {
    if ( argv[iarg] == 0 ) break;
    string name = argv[iarg];
    if ( name == "all" || name == "All" || name == "ALL" )  {
      const DetElement::Children& children = lcdd.world().children();
      for (DetElement::Children::const_iterator i=children.begin(); i!=children.end(); ++i)  {
        DetElement sdet = (*i).second;
        printout(INFO,"DD4hepVolumeMgrTest","++ Processing subdetector: %s",sdet.name());
        VolIDTest test(lcdd,sdet,99);
      }
      return 1;
    }
    printout(INFO,"DD4hepVolumeMgrTest","++ Processing subdetector: %s",name.c_str());
    VolIDTest test(lcdd,lcdd.detector(name),99);
  }
  return 1;
}

DECLARE_APPLY(DD4hepVolumeMgrTest,VolIDTest::run)
