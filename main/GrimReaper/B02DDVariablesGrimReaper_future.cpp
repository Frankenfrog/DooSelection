/******************************************/
// B02DDVariablesGrimReaper.cpp
//
// Standalone GrimReaper that adds a lot of
// common variables to trees.
//
// Author: Frank Meier
// Date: 2014-11-07
/******************************************/

// from STL
#include <tuple>
#include <list>

// from ROOT
#include "TRandom3.h"
#include "TCut.h"

// from DooCore
#include "doocore/io/MsgStream.h"
#include "doocore/config/Summary.h"

// from DooSelection
#include "dooselection/reducer/Reducer.h"
#include "dooselection/reducer/ReducerLeaf.h"
#include "dooselection/reducer/KinematicReducerLeaf.h"

using namespace dooselection::reducer;
using namespace doocore::io;

// forward declarations
// typedef tuple: head, daughters, stable particles, isMC, isFlat
typedef std::tuple<std::string, std::list<std::string>, std::list<std::string>, bool, bool> cfg_tuple;
cfg_tuple Configure(Reducer* _rdcr, std::string& _channel);
void MassLeaves(Reducer* _rdcr, cfg_tuple& cfg);
void TimeLeaves(Reducer* _rdcr, cfg_tuple& cfg);
void TriggerLeaves(Reducer* _rdcr);
void VetoLeaves(Reducer* _rdcr, cfg_tuple& cfg);
void AuxiliaryLeaves(Reducer* _rdcr, cfg_tuple& cfg);

int main(int argc, char * argv[]){
  sinfo << "-info-  \t" << "B02DDVariablesGrimReaper \t" << "Welcome!" << endmsg;
  std::string inputfile, inputtree, outputfile, outputtree, decay_channel;
  if (argc == 5){
    inputfile = argv[1];
    inputtree = argv[2];
    outputfile = argv[3];
    outputtree = argv[4];
    decay_channel = "B02DD";
  }
  else{
    serr << "-ERROR- \t" << "B02DDVariablesGrimReaper \t" << "Parameters needed:" << endmsg;
    serr << "-ERROR- \t" << "B02DDVariablesGrimReaper \t"<< "input_file_name input_tree_name output_file_name output_tree_name" << endmsg;
    return 1;
  }

  Reducer* reducer = new Reducer();
  doocore::config::Summary& summary = doocore::config::Summary::GetInstance();
  summary.AddSection("I/O");
  summary.Add("Input file", inputfile);
  summary.Add("Input tree", inputtree);
  summary.Add("Output file", outputfile);
  summary.Add("Output file", outputtree);

  // reducer part
  reducer->set_input_file_path(inputfile);
  reducer->set_input_tree_path(inputtree);
  reducer->set_output_file_path(outputfile);
  reducer->set_output_tree_path(outputtree);

  reducer->Initialize();

  // config
  cfg_tuple cfg = Configure(reducer, decay_channel);

  // add leaves  
  summary.AddSection("Added leaves");
  MassLeaves(reducer, cfg);
  TimeLeaves(reducer, cfg);
  TriggerLeaves(reducer);
  VetoLeaves(reducer, cfg);
  AuxiliaryLeaves(reducer, cfg);

  reducer->Run();
  reducer->Finalize();
}

cfg_tuple Configure(Reducer* _rdcr, std::string& _channel){
  doocore::config::Summary& summary = doocore::config::Summary::GetInstance();
  summary.AddSection("Channel");
  // typedef tuple: head, daughters, stable particles, isMC, isFlat
  std::string head ="";
  std::list<std::string> daughters;
  std::list<std::string> stable_particles;
  bool isMC = false;
  bool isFlat = false;
  if (_channel == "B02DD"){
    head = "B0";
    daughters.push_back("Dplus");
    daughters.push_back("Dminus");
    stable_particles.push_back("Dplus_Kminus_or_piminus");
    stable_particles.push_back("Dplus_piplus_or_Kplus_One");
    stable_particles.push_back("Dplus_piplus_or_Kplus_Two");
    stable_particles.push_back("Dminus_Kplus_or_piplus");
    stable_particles.push_back("Dminus_piminus_or_Kminus_One");
    stable_particles.push_back("Dminus_piminus_or_Kminus_Two");
    isMC = _rdcr->LeafExists(head+"_BKGCAT");
    isFlat = (_rdcr->LeafExists("flat_array_index") || _rdcr->LeafExists("idxPV"));
  }
  else{
    serr << "-ERROR- \t" << "B02DDVariablesGrimReaper \t" << "No valid decay channel. Possible decay channels are:" << endmsg;
    serr << "-ERROR- \t" << "B02DDVariablesGrimReaper \t" << "- B02DD" << endmsg;
  }
  summary.Add("Name", _channel);
  summary.Add("Head", head);
  for (std::list<std::string>::iterator it = daughters.begin(); it != daughters.end(); ++it){
    summary.Add("Daughter", *it);
  }
  for (std::list<std::string>::iterator it = stable_particles.begin(); it != stable_particles.end(); ++it){
    summary.Add("Stable", *it);
  }
  summary.AddSection("Data Type");
  summary.Add("MC", isMC);
  summary.Add("Flat", isFlat);

  if (isFlat) sinfo << "-info-  \t" << "You are running the reducer over a flat tuple!" << endmsg;
  if (isMC) sinfo << "-info-  \t" << "You are running the reducer over a MC tuple!" << endmsg;

  return std::make_tuple(head, daughters, stable_particles, isMC, isFlat);
}

void MassLeaves(Reducer* _rdcr, cfg_tuple& cfg){
  doocore::config::Summary& summary = doocore::config::Summary::GetInstance();
  // handle flattened tuples
  std::string flat_suffix = "";
  if (std::get<4>(cfg)) flat_suffix = "_flat";
  sinfo << "-info-  \t" << "Setting flat suffix to " << flat_suffix << endmsg;

  // create copies of mass observables for different fit constraints
  // plus the 'nominal' mass observable depending on the following hierarchy
  std::string main_observable_constraint = "PV";
  std::string main_observable_constraint_error = "PV";
  if (_rdcr->LeafExists(std::get<0>(cfg)+"_FitDDPVConst_M"+flat_suffix)) {
    if (main_observable_constraint == "DDPV") main_observable_constraint = "FitDDPVConst_M";
    if (main_observable_constraint_error == "DDPV") main_observable_constraint_error = "FitDDPVConst_MERR";
    _rdcr->CreateDoubleCopyLeaf("obsMassDDPVConst", _rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_FitDDPVConst_M"+flat_suffix));
    _rdcr->CreateDoubleCopyLeaf("obsMassErrDDPVConst", _rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_FitDDPVConst_MERR"+flat_suffix));
  } 
  if (_rdcr->LeafExists(std::get<0>(cfg)+"_FitPVConst_M"+flat_suffix)) {
    if (main_observable_constraint == "PV") main_observable_constraint = "FitPVConst_M";
    if (main_observable_constraint_error == "PV") main_observable_constraint_error = "FitPVConst_MERR";
    _rdcr->CreateDoubleCopyLeaf("obsMassPVConst", _rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_FitPVConst_M"+flat_suffix));
    _rdcr->CreateDoubleCopyLeaf("obsMassErrPVConst", _rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_FitPVConst_MERR"+flat_suffix));
    _rdcr->CreateDoubleCopyLeaf("obsMassDauOne", _rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_FitPVConst_Dplus_M"+flat_suffix));
    _rdcr->CreateDoubleCopyLeaf("obsMassErrDauOne", _rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_FitPVConst_Dplus_MERR"+flat_suffix));
    _rdcr->CreateDoubleCopyLeaf("obsMassDauTwo", _rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_FitPVConst_Dminus_M"+flat_suffix));
    _rdcr->CreateDoubleCopyLeaf("obsMassErrDauTwo", _rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_FitPVConst_Dminus_MERR"+flat_suffix));
  } 
  if (_rdcr->LeafExists(std::get<0>(cfg)+"_LOKI_MASS_DDConstr")) {
    if (main_observable_constraint == "LOKI") main_observable_constraint = "LOKI_MASS_DDConstr";
    if (main_observable_constraint_error == "LOKI") main_observable_constraint_error = "LOKI_MASSERR_DDConstr";
    _rdcr->CreateDoubleCopyLeaf("obsMassLokiDDPVConst", _rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_LOKI_MASS_DDConstr"));
    _rdcr->CreateDoubleCopyLeaf("obsMassErrLokiDDPVConst", _rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_LOKI_MASSERR_DDConstr"));
    _rdcr->CreateDoubleCopyLeaf("obsMassLokiDauOne", _rdcr->GetInterimLeafByName("Dplus_MM"));
    _rdcr->CreateDoubleCopyLeaf("obsMassErrLokiDauOne", _rdcr->GetInterimLeafByName("Dplus_MMERR"));
    _rdcr->CreateDoubleCopyLeaf("obsMassLokiDauTwo", _rdcr->GetInterimLeafByName("Dminus_MM"));
    _rdcr->CreateDoubleCopyLeaf("obsMassErrLokiDauTwo", _rdcr->GetInterimLeafByName("Dminus_MMERR"));
  } 
  _rdcr->CreateDoubleCopyLeaf("obsMass", _rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_"+main_observable_constraint+flat_suffix));
  _rdcr->CreateDoubleCopyLeaf("obsMassErr", _rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_"+main_observable_constraint_error+flat_suffix));
  summary.Add("obsMass fit constraints", main_observable_constraint);
}

void TimeLeaves(Reducer* _rdcr, cfg_tuple& cfg){
  doocore::config::Summary& summary = doocore::config::Summary::GetInstance();
  // handle flattened tuples
  std::string flat_suffix = "";
  std::string fit_constraints = "";
  if (std::get<4>(cfg)) flat_suffix = "_flat";

  ReducerLeaf<Double_t>* tau_leaf_ptr = NULL;
  ReducerLeaf<Double_t>* tau_true_leaf_ptr = NULL;
  ReducerLeaf<Double_t>* tau_err_leaf_ptr = NULL;
  ReducerLeaf<Double_t>* tau_true_err_leaf_ptr = NULL;

  if (_rdcr->LeafExists(std::get<0>(cfg)+"_FitPVConst_tau")) {
    fit_constraints = "PVConst";
    tau_leaf_ptr = &_rdcr->CreateDoubleCopyLeaf("obsTime", _rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_FitPVConst_tau"+flat_suffix), 1000.0);
    tau_err_leaf_ptr = &_rdcr->CreateDoubleCopyLeaf("obsTimeErr", _rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_FitPVConst_tauErr"+flat_suffix), 1000.0);
    ReducerLeaf<Double_t>& varDplusTauSignificance = _rdcr->CreateDoubleLeaf("varDplusTauSignificance", -999999.);
    varDplusTauSignificance.Divide(_rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_FitPVConst_Dplus_tau"+flat_suffix), _rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_FitPVConst_Dplus_tauErr"+flat_suffix));
    ReducerLeaf<Double_t>& varDminusTauSignificance = _rdcr->CreateDoubleLeaf("varDminusTauSignificance", -999999.);
    varDminusTauSignificance.Divide(_rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_FitPVConst_Dminus_tau"+flat_suffix), _rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_FitPVConst_Dminus_tauErr"+flat_suffix));
    _rdcr->CreateDoubleLeaf("varDMinTauSignificance", -999999.).Minimum(varDplusTauSignificance, varDminusTauSignificance);
  }
  else if (_rdcr->LeafExists(std::get<0>(cfg)+"_LOKI_DTF_CTAU")){
    fit_constraints = "LOKI";
    tau_leaf_ptr = &_rdcr->CreateDoubleCopyLeaf("obsTime", _rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_LOKI_DTF_CTAU"), 1.0/0.299792458);
    tau_err_leaf_ptr = &_rdcr->CreateDoubleCopyLeaf("obsTimeErr", _rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_LOKI_DTF_CTAUERR"), 1.0/0.299792458);
  }

  if (_rdcr->LeafExists(std::get<0>(cfg)+"_BKGCAT")){
    tau_true_leaf_ptr = &_rdcr->CreateDoubleCopyLeaf("obsTime_True", _rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_TRUETAU"), 1000.0);
    
    // the "true" time error is defined as "reconstructed decay time" - "true decay time"
    tau_true_err_leaf_ptr = &_rdcr->CreateDoubleLeaf("obsTimeErr_True", 1000.0);
    if (_rdcr->LeafExists(std::get<0>(cfg)+"_FitPVConst_tau")) {
      tau_true_err_leaf_ptr->Add(_rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_FitPVConst_tau"+flat_suffix), _rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_TRUETAU"), 1.0, -1.0);
    }
    else if (_rdcr->LeafExists(std::get<0>(cfg)+"_LOKI_DTF_CTAU")){
      tau_true_err_leaf_ptr->Add(_rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_LOKI_DTF_CTAU"), _rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_TRUETAU"), 1.0/0.299792458, -1.0);      
    }
  }

  summary.Add("Time fit constraints", fit_constraints);
}

void TriggerLeaves(Reducer* _rdcr){
  // trigger categories
  _rdcr->CreateIntCopyLeaf("catTriggerL0GlobalTOS", _rdcr->GetInterimLeafByName("B0_L0Global_TOS"));
  _rdcr->CreateIntCopyLeaf("catTriggerHlt1GlobalTOS", _rdcr->GetInterimLeafByName("B0_Hlt1Global_TOS"));
  _rdcr->CreateIntCopyLeaf("catTriggerHlt2GlobalTOS", _rdcr->GetInterimLeafByName("B0_Hlt2Global_TOS"));
  _rdcr->CreateIntCopyLeaf("catTriggerHlt2Topo2BodyBBDTTOS", _rdcr->GetInterimLeafByName("B0_Hlt2Topo2BodyBBDTDecision_TOS"));
  _rdcr->CreateIntCopyLeaf("catTriggerHlt2Topo3BodyBBDTTOS", _rdcr->GetInterimLeafByName("B0_Hlt2Topo3BodyBBDTDecision_TOS"));
  _rdcr->CreateIntCopyLeaf("catTriggerHlt2Topo4BodyBBDTTOS", _rdcr->GetInterimLeafByName("B0_Hlt2Topo4BodyBBDTDecision_TOS"));

  ReducerLeaf<Int_t>& catTriggerSetTopo234bodybbdt = _rdcr->CreateIntLeaf("catTriggerSetTopo234BodyBBDT", -10);
      TCut trigger_set_topo234bodybbdt = "B0_Hlt2Topo2BodyBBDTDecision_TOS==1||B0_Hlt2Topo3BodyBBDTDecision_TOS==1||B0_Hlt2Topo4BodyBBDTDecision_TOS==1";
      catTriggerSetTopo234bodybbdt.AddCondition("triggered", trigger_set_topo234bodybbdt.GetTitle(), 1);
      catTriggerSetTopo234bodybbdt.AddCondition("not_triggered", (!trigger_set_topo234bodybbdt).GetTitle(), 0);
}

void VetoLeaves(Reducer* _rdcr, cfg_tuple& cfg){
  doocore::config::Summary& summary = doocore::config::Summary::GetInstance();
  // handle flattened tuples
  std::string flat_suffix = "";
  if (std::get<4>(cfg)) flat_suffix = "_flat";

  // veto leafs
  std::string Dplus_px, Dplus_py, Dplus_pz;
  std::string Dminus_px, Dminus_py, Dminus_pz;
  std::string piplus_px, piplus_py, piplus_pz;
  std::string piplus0_px, piplus0_py, piplus0_pz;
  std::string piminus_px, piminus_py, piminus_pz;
  std::string piminus0_px, piminus0_py, piminus0_pz;
  std::string Kplus_px, Kplus_py, Kplus_pz;
  std::string Kminus_px, Kminus_py, Kminus_pz;

  std::string mass_hypo_constraints = "";
  if (_rdcr->LeafExists(std::get<0>(cfg)+"_FitPVConst_Dplus_P0_PX")){
    Dplus_px    = std::get<0>(cfg)+"_FitPVConst_Dplus_PX"+flat_suffix;
    Dplus_py    = std::get<0>(cfg)+"_FitPVConst_Dplus_PY"+flat_suffix;
    Dplus_pz    = std::get<0>(cfg)+"_FitPVConst_Dplus_PZ"+flat_suffix;
    Dminus_px   = std::get<0>(cfg)+"_FitPVConst_Dminus_PX"+flat_suffix;
    Dminus_py   = std::get<0>(cfg)+"_FitPVConst_Dminus_PY"+flat_suffix;
    Dminus_pz   = std::get<0>(cfg)+"_FitPVConst_Dminus_PZ"+flat_suffix;
    Kplus_px    = std::get<0>(cfg)+"_FitPVConst_Dminus_P2_PX"+flat_suffix;
    Kplus_py    = std::get<0>(cfg)+"_FitPVConst_Dminus_P2_PY"+flat_suffix;
    Kplus_pz    = std::get<0>(cfg)+"_FitPVConst_Dminus_P2_PZ"+flat_suffix;
    piminus_px  = std::get<0>(cfg)+"_FitPVConst_Dminus_P0_PX"+flat_suffix;
    piminus_py  = std::get<0>(cfg)+"_FitPVConst_Dminus_P0_PY"+flat_suffix;
    piminus_pz  = std::get<0>(cfg)+"_FitPVConst_Dminus_P0_PZ"+flat_suffix;
    piminus0_px = std::get<0>(cfg)+"_FitPVConst_Dminus_P1_PX"+flat_suffix;
    piminus0_py = std::get<0>(cfg)+"_FitPVConst_Dminus_P1_PY"+flat_suffix;
    piminus0_pz = std::get<0>(cfg)+"_FitPVConst_Dminus_P1_PZ"+flat_suffix;
    Kminus_px   = std::get<0>(cfg)+"_FitPVConst_Dplus_P2_PX"+flat_suffix;
    Kminus_py   = std::get<0>(cfg)+"_FitPVConst_Dplus_P2_PY"+flat_suffix;
    Kminus_pz   = std::get<0>(cfg)+"_FitPVConst_Dplus_P2_PZ"+flat_suffix;
    piplus_px   = std::get<0>(cfg)+"_FitPVConst_Dplus_P0_PX"+flat_suffix;
    piplus_py   = std::get<0>(cfg)+"_FitPVConst_Dplus_P0_PY"+flat_suffix;
    piplus_pz   = std::get<0>(cfg)+"_FitPVConst_Dplus_P0_PZ"+flat_suffix;
    piplus0_px  = std::get<0>(cfg)+"_FitPVConst_Dplus_P1_PX"+flat_suffix;
    piplus0_py  = std::get<0>(cfg)+"_FitPVConst_Dplus_P1_PY"+flat_suffix;
    piplus0_pz  = std::get<0>(cfg)+"_FitPVConst_Dplus_P1_PZ"+flat_suffix;
    mass_hypo_constraints = "PV";
  }
  else if (_rdcr->LeafExists("Dplus_Kminus_or_piminus_PX")){
    Dplus_px    = "Dplus_PX";
    Dplus_py    = "Dplus_PY";
    Dplus_pz    = "Dplus_PZ";
    Dminus_px   = "Dminus_PX";
    Dminus_py   = "Dminus_PY";
    Dminus_pz   = "Dminus_PZ";
    Kplus_px    = "Dminus_Kplus_or_piplus_PX";
    Kplus_py    = "Dminus_Kplus_or_piplus_PY";
    Kplus_pz    = "Dminus_Kplus_or_piplus_PZ";
    Kminus_px   = "Dplus_Kminus_or_piminus_PX";
    Kminus_py   = "Dplus_Kminus_or_piminus_PY";
    Kminus_pz   = "Dplus_Kminus_or_piminus_PZ";
    piminus_px  = "Dminus_piminus_or_Kminus_One_PX";
    piminus_py  = "Dminus_piminus_or_Kminus_One_PY";
    piminus_pz  = "Dminus_piminus_or_Kminus_One_PZ";
    piminus0_px = "Dminus_piminus_or_Kminus_Two_PX";
    piminus0_py = "Dminus_piminus_or_Kminus_Two_PY";
    piminus0_pz = "Dminus_piminus_or_Kminus_Two_PZ";
    piplus_px   = "Dplus_piplus_or_Kplus_One_PX";
    piplus_py   = "Dplus_piplus_or_Kplus_One_PY";
    piplus_pz   = "Dplus_piplus_or_Kplus_One_PZ";
    piplus0_px  = "Dplus_piplus_or_Kplus_Two_PX";
    piplus0_py  = "Dplus_piplus_or_Kplus_Two_PY";
    piplus0_pz  = "Dplus_piplus_or_Kplus_Two_PZ";
    mass_hypo_constraints = "NoConstr";
  }
  
  if (mass_hypo_constraints!=""){
    // mass hypotheses
    KinematicReducerLeaf<Double_t>* varDplusMassHypo_Kpipi = new KinematicReducerLeaf<Double_t>("varDplusMassHypo_Kpipi", "varDplusMassHypo_Kpipi", "Double_t", NULL);
    varDplusMassHypo_Kpipi->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kminus_px),
        _rdcr->GetInterimLeafByName(Kminus_py),
        _rdcr->GetInterimLeafByName(Kminus_pz),
        493.677,
        _rdcr->GetInterimLeafByName(piplus_px),
        _rdcr->GetInterimLeafByName(piplus_py),
        _rdcr->GetInterimLeafByName(piplus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piplus0_px),
        _rdcr->GetInterimLeafByName(piplus0_py),
        _rdcr->GetInterimLeafByName(piplus0_pz),
        139.57018);
    _rdcr->RegisterDoubleLeaf(varDplusMassHypo_Kpipi);

    KinematicReducerLeaf<Double_t>* varDminusMassHypo_Kpipi = new KinematicReducerLeaf<Double_t>("varDminusMassHypo_Kpipi", "varDminusMassHypo_Kpipi", "Double_t", NULL);
    varDminusMassHypo_Kpipi->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kplus_px),
        _rdcr->GetInterimLeafByName(Kplus_py),
        _rdcr->GetInterimLeafByName(Kplus_pz),
        493.677,
        _rdcr->GetInterimLeafByName(piminus_px),
        _rdcr->GetInterimLeafByName(piminus_py),
        _rdcr->GetInterimLeafByName(piminus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piminus0_px),
        _rdcr->GetInterimLeafByName(piminus0_py),
        _rdcr->GetInterimLeafByName(piminus0_pz),
        139.57018);
    _rdcr->RegisterDoubleLeaf(varDminusMassHypo_Kpipi);

    KinematicReducerLeaf<Double_t>* varDplusMassHypo_KKpi = new KinematicReducerLeaf<Double_t>("varDplusMassHypo_KKpi", "varDplusMassHypo_KKpi", "Double_t", NULL);
    varDplusMassHypo_KKpi->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kminus_px),
        _rdcr->GetInterimLeafByName(Kminus_py),
        _rdcr->GetInterimLeafByName(Kminus_pz),
        493.677,
        _rdcr->GetInterimLeafByName(piplus_px),
        _rdcr->GetInterimLeafByName(piplus_py),
        _rdcr->GetInterimLeafByName(piplus_pz),
        493.677,
        _rdcr->GetInterimLeafByName(piplus0_px),
        _rdcr->GetInterimLeafByName(piplus0_py),
        _rdcr->GetInterimLeafByName(piplus0_pz),
        139.57018);
    _rdcr->RegisterDoubleLeaf(varDplusMassHypo_KKpi);

    KinematicReducerLeaf<Double_t>* varDminusMassHypo_KKpi = new KinematicReducerLeaf<Double_t>("varDminusMassHypo_KKpi", "varDminusMassHypo_KKpi", "Double_t", NULL);
    varDminusMassHypo_KKpi->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kplus_px),
        _rdcr->GetInterimLeafByName(Kplus_py),
        _rdcr->GetInterimLeafByName(Kplus_pz),
        493.677,
        _rdcr->GetInterimLeafByName(piminus_px),
        _rdcr->GetInterimLeafByName(piminus_py),
        _rdcr->GetInterimLeafByName(piminus_pz),
        493.677,
        _rdcr->GetInterimLeafByName(piminus0_px),
        _rdcr->GetInterimLeafByName(piminus0_py),
        _rdcr->GetInterimLeafByName(piminus0_pz),
        139.57018);
    _rdcr->RegisterDoubleLeaf(varDminusMassHypo_KKpi);

  KinematicReducerLeaf<Double_t>* varDplusMassHypo_KpiK = new KinematicReducerLeaf<Double_t>("varDplusMassHypo_KpiK", "varDplusMassHypo_KpiK", "Double_t", NULL);
    varDplusMassHypo_KpiK->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kminus_px),
        _rdcr->GetInterimLeafByName(Kminus_py),
        _rdcr->GetInterimLeafByName(Kminus_pz),
        493.677,
        _rdcr->GetInterimLeafByName(piplus_px),
        _rdcr->GetInterimLeafByName(piplus_py),
        _rdcr->GetInterimLeafByName(piplus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piplus0_px),
        _rdcr->GetInterimLeafByName(piplus0_py),
        _rdcr->GetInterimLeafByName(piplus0_pz),
        493.677);
    _rdcr->RegisterDoubleLeaf(varDplusMassHypo_KpiK);

    KinematicReducerLeaf<Double_t>* varDminusMassHypo_KpiK = new KinematicReducerLeaf<Double_t>("varDminusMassHypo_KpiK", "varDminusMassHypo_KpiK", "Double_t", NULL);
    varDminusMassHypo_KpiK->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kplus_px),
        _rdcr->GetInterimLeafByName(Kplus_py),
        _rdcr->GetInterimLeafByName(Kplus_pz),
        493.677,
        _rdcr->GetInterimLeafByName(piminus_px),
        _rdcr->GetInterimLeafByName(piminus_py),
        _rdcr->GetInterimLeafByName(piminus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piminus0_px),
        _rdcr->GetInterimLeafByName(piminus0_py),
        _rdcr->GetInterimLeafByName(piminus0_pz),
        493.677);
    _rdcr->RegisterDoubleLeaf(varDminusMassHypo_KpiK);

    KinematicReducerLeaf<Double_t>* varDplusMassHypo_KKK = new KinematicReducerLeaf<Double_t>("varDplusMassHypo_KKK", "varDplusMassHypo_KKK", "Double_t", NULL);
    varDplusMassHypo_KKK->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kminus_px),
        _rdcr->GetInterimLeafByName(Kminus_py),
        _rdcr->GetInterimLeafByName(Kminus_pz),
        493.677,
        _rdcr->GetInterimLeafByName(piplus_px),
        _rdcr->GetInterimLeafByName(piplus_py),
        _rdcr->GetInterimLeafByName(piplus_pz),
        493.677,
        _rdcr->GetInterimLeafByName(piplus0_px),
        _rdcr->GetInterimLeafByName(piplus0_py),
        _rdcr->GetInterimLeafByName(piplus0_pz),
        493.677);
    _rdcr->RegisterDoubleLeaf(varDplusMassHypo_KKK);

    KinematicReducerLeaf<Double_t>* varDminusMassHypo_KKK = new KinematicReducerLeaf<Double_t>("varDminusMassHypo_KKK", "varDminusMassHypo_KKK", "Double_t", NULL);
    varDminusMassHypo_KKK->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kplus_px),
        _rdcr->GetInterimLeafByName(Kplus_py),
        _rdcr->GetInterimLeafByName(Kplus_pz),
        493.677,
        _rdcr->GetInterimLeafByName(piminus_px),
        _rdcr->GetInterimLeafByName(piminus_py),
        _rdcr->GetInterimLeafByName(piminus_pz),
        493.677,
        _rdcr->GetInterimLeafByName(piminus0_px),
        _rdcr->GetInterimLeafByName(piminus0_py),
        _rdcr->GetInterimLeafByName(piminus0_pz),
        493.677);
    _rdcr->RegisterDoubleLeaf(varDminusMassHypo_KKK);

    KinematicReducerLeaf<Double_t>* varDplusMassHypo_pipipi = new KinematicReducerLeaf<Double_t>("varDplusMassHypo_pipipi", "varDplusMassHypo_pipipi", "Double_t", NULL);
    varDplusMassHypo_pipipi->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kminus_px),
        _rdcr->GetInterimLeafByName(Kminus_py),
        _rdcr->GetInterimLeafByName(Kminus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piplus_px),
        _rdcr->GetInterimLeafByName(piplus_py),
        _rdcr->GetInterimLeafByName(piplus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piplus0_px),
        _rdcr->GetInterimLeafByName(piplus0_py),
        _rdcr->GetInterimLeafByName(piplus0_pz),
        139.57018);
    _rdcr->RegisterDoubleLeaf(varDplusMassHypo_pipipi);

    KinematicReducerLeaf<Double_t>* varDminusMassHypo_pipipi = new KinematicReducerLeaf<Double_t>("varDminusMassHypo_pipipi", "varDminusMassHypo_pipipi", "Double_t", NULL);
    varDminusMassHypo_pipipi->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kplus_px),
        _rdcr->GetInterimLeafByName(Kplus_py),
        _rdcr->GetInterimLeafByName(Kplus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piminus_px),
        _rdcr->GetInterimLeafByName(piminus_py),
        _rdcr->GetInterimLeafByName(piminus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piminus0_px),
        _rdcr->GetInterimLeafByName(piminus0_py),
        _rdcr->GetInterimLeafByName(piminus0_pz),
        139.57018);
    _rdcr->RegisterDoubleLeaf(varDminusMassHypo_pipipi);

    KinematicReducerLeaf<Double_t>* varDplusMassHypo_pipiK = new KinematicReducerLeaf<Double_t>("varDplusMassHypo_pipiK", "varDplusMassHypo_pipiK", "Double_t", NULL);
    varDplusMassHypo_pipiK->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kminus_px),
        _rdcr->GetInterimLeafByName(Kminus_py),
        _rdcr->GetInterimLeafByName(Kminus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piplus_px),
        _rdcr->GetInterimLeafByName(piplus_py),
        _rdcr->GetInterimLeafByName(piplus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piplus0_px),
        _rdcr->GetInterimLeafByName(piplus0_py),
        _rdcr->GetInterimLeafByName(piplus0_pz),
        493.677);
    _rdcr->RegisterDoubleLeaf(varDplusMassHypo_pipiK);

    KinematicReducerLeaf<Double_t>* varDminusMassHypo_pipiK = new KinematicReducerLeaf<Double_t>("varDminusMassHypo_pipiK", "varDminusMassHypo_pipiK", "Double_t", NULL);
    varDminusMassHypo_pipiK->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kplus_px),
        _rdcr->GetInterimLeafByName(Kplus_py),
        _rdcr->GetInterimLeafByName(Kplus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piminus_px),
        _rdcr->GetInterimLeafByName(piminus_py),
        _rdcr->GetInterimLeafByName(piminus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piminus0_px),
        _rdcr->GetInterimLeafByName(piminus0_py),
        _rdcr->GetInterimLeafByName(piminus0_pz),
        493.677);
    _rdcr->RegisterDoubleLeaf(varDminusMassHypo_pipiK);

    KinematicReducerLeaf<Double_t>* varDplusMassHypo_piKpi = new KinematicReducerLeaf<Double_t>("varDplusMassHypo_piKpi", "varDplusMassHypo_piKpi", "Double_t", NULL);
    varDplusMassHypo_piKpi->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kminus_px),
        _rdcr->GetInterimLeafByName(Kminus_py),
        _rdcr->GetInterimLeafByName(Kminus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piplus_px),
        _rdcr->GetInterimLeafByName(piplus_py),
        _rdcr->GetInterimLeafByName(piplus_pz),
        493.677,
        _rdcr->GetInterimLeafByName(piplus0_px),
        _rdcr->GetInterimLeafByName(piplus0_py),
        _rdcr->GetInterimLeafByName(piplus0_pz),
        139.57018);
    _rdcr->RegisterDoubleLeaf(varDplusMassHypo_piKpi);

    KinematicReducerLeaf<Double_t>* varDminusMassHypo_piKpi = new KinematicReducerLeaf<Double_t>("varDminusMassHypo_piKpi", "varDminusMassHypo_piKpi", "Double_t", NULL);
    varDminusMassHypo_piKpi->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kplus_px),
        _rdcr->GetInterimLeafByName(Kplus_py),
        _rdcr->GetInterimLeafByName(Kplus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piminus_px),
        _rdcr->GetInterimLeafByName(piminus_py),
        _rdcr->GetInterimLeafByName(piminus_pz),
        493.677,
        _rdcr->GetInterimLeafByName(piminus0_px),
        _rdcr->GetInterimLeafByName(piminus0_py),
        _rdcr->GetInterimLeafByName(piminus0_pz),
        139.57018);
    _rdcr->RegisterDoubleLeaf(varDminusMassHypo_piKpi);

    KinematicReducerLeaf<Double_t>* varDplusMassHypo_piKK = new KinematicReducerLeaf<Double_t>("varDplusMassHypo_piKK", "varDplusMassHypo_piKK", "Double_t", NULL);
    varDplusMassHypo_piKK->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kminus_px),
        _rdcr->GetInterimLeafByName(Kminus_py),
        _rdcr->GetInterimLeafByName(Kminus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piplus_px),
        _rdcr->GetInterimLeafByName(piplus_py),
        _rdcr->GetInterimLeafByName(piplus_pz),
        493.677,
        _rdcr->GetInterimLeafByName(piplus0_px),
        _rdcr->GetInterimLeafByName(piplus0_py),
        _rdcr->GetInterimLeafByName(piplus0_pz),
        493.677);
    _rdcr->RegisterDoubleLeaf(varDplusMassHypo_piKK);

    KinematicReducerLeaf<Double_t>* varDminusMassHypo_piKK = new KinematicReducerLeaf<Double_t>("varDminusMassHypo_piKK", "varDminusMassHypo_piKK", "Double_t", NULL);
    varDminusMassHypo_piKK->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kplus_px),
        _rdcr->GetInterimLeafByName(Kplus_py),
        _rdcr->GetInterimLeafByName(Kplus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piminus_px),
        _rdcr->GetInterimLeafByName(piminus_py),
        _rdcr->GetInterimLeafByName(piminus_pz),
        493.677,
        _rdcr->GetInterimLeafByName(piminus0_px),
        _rdcr->GetInterimLeafByName(piminus0_py),
        _rdcr->GetInterimLeafByName(piminus0_pz),
        493.677);
    _rdcr->RegisterDoubleLeaf(varDminusMassHypo_piKK);

    KinematicReducerLeaf<Double_t>* varDplusMassHypo_Kppi = new KinematicReducerLeaf<Double_t>("varDplusMassHypo_Kppi", "varDplusMassHypo_Kppi", "Double_t", NULL);
    varDplusMassHypo_Kppi->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kminus_px),
        _rdcr->GetInterimLeafByName(Kminus_py),
        _rdcr->GetInterimLeafByName(Kminus_pz),
        493.677,
        _rdcr->GetInterimLeafByName(piplus_px),
        _rdcr->GetInterimLeafByName(piplus_py),
        _rdcr->GetInterimLeafByName(piplus_pz),
        938.272046,
        _rdcr->GetInterimLeafByName(piplus0_px),
        _rdcr->GetInterimLeafByName(piplus0_py),
        _rdcr->GetInterimLeafByName(piplus0_pz),
        139.57018);
    _rdcr->RegisterDoubleLeaf(varDplusMassHypo_Kppi);

    KinematicReducerLeaf<Double_t>* varDminusMassHypo_Kppi = new KinematicReducerLeaf<Double_t>("varDminusMassHypo_Kppi", "varDminusMassHypo_Kppi", "Double_t", NULL);
    varDminusMassHypo_Kppi->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kplus_px),
        _rdcr->GetInterimLeafByName(Kplus_py),
        _rdcr->GetInterimLeafByName(Kplus_pz),
        493.677,
        _rdcr->GetInterimLeafByName(piminus_px),
        _rdcr->GetInterimLeafByName(piminus_py),
        _rdcr->GetInterimLeafByName(piminus_pz),
        938.272046,
        _rdcr->GetInterimLeafByName(piminus0_px),
        _rdcr->GetInterimLeafByName(piminus0_py),
        _rdcr->GetInterimLeafByName(piminus0_pz),
        139.57018);
    _rdcr->RegisterDoubleLeaf(varDminusMassHypo_Kppi);

    KinematicReducerLeaf<Double_t>* varDplusMassHypo_Kpip = new KinematicReducerLeaf<Double_t>("varDplusMassHypo_Kpip", "varDplusMassHypo_Kpip", "Double_t", NULL);
    varDplusMassHypo_Kpip->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kminus_px),
        _rdcr->GetInterimLeafByName(Kminus_py),
        _rdcr->GetInterimLeafByName(Kminus_pz),
        493.677,
        _rdcr->GetInterimLeafByName(piplus_px),
        _rdcr->GetInterimLeafByName(piplus_py),
        _rdcr->GetInterimLeafByName(piplus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piplus0_px),
        _rdcr->GetInterimLeafByName(piplus0_py),
        _rdcr->GetInterimLeafByName(piplus0_pz),
        938.272046);
    _rdcr->RegisterDoubleLeaf(varDplusMassHypo_Kpip);

    KinematicReducerLeaf<Double_t>* varDminusMassHypo_Kpip = new KinematicReducerLeaf<Double_t>("varDminusMassHypo_Kpip", "varDminusMassHypo_Kpip", "Double_t", NULL);
    varDminusMassHypo_Kpip->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kplus_px),
        _rdcr->GetInterimLeafByName(Kplus_py),
        _rdcr->GetInterimLeafByName(Kplus_pz),
        493.677,
        _rdcr->GetInterimLeafByName(piminus_px),
        _rdcr->GetInterimLeafByName(piminus_py),
        _rdcr->GetInterimLeafByName(piminus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piminus0_px),
        _rdcr->GetInterimLeafByName(piminus0_py),
        _rdcr->GetInterimLeafByName(piminus0_pz),
        938.272046);
    _rdcr->RegisterDoubleLeaf(varDminusMassHypo_Kpip);

    KinematicReducerLeaf<Double_t>* varDplusMassHypo_pKpi = new KinematicReducerLeaf<Double_t>("varDplusMassHypo_pKpi", "varDplusMassHypo_pKpi", "Double_t", NULL);
    varDplusMassHypo_pKpi->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kminus_px),
        _rdcr->GetInterimLeafByName(Kminus_py),
        _rdcr->GetInterimLeafByName(Kminus_pz),
        938.272046,
        _rdcr->GetInterimLeafByName(piplus_px),
        _rdcr->GetInterimLeafByName(piplus_py),
        _rdcr->GetInterimLeafByName(piplus_pz),
        493.677,
        _rdcr->GetInterimLeafByName(piplus0_px),
        _rdcr->GetInterimLeafByName(piplus0_py),
        _rdcr->GetInterimLeafByName(piplus0_pz),
        139.57018);
    _rdcr->RegisterDoubleLeaf(varDplusMassHypo_pKpi);

    KinematicReducerLeaf<Double_t>* varDminusMassHypo_pKpi = new KinematicReducerLeaf<Double_t>("varDminusMassHypo_pKpi", "varDminusMassHypo_pKpi", "Double_t", NULL);
    varDminusMassHypo_pKpi->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kplus_px),
        _rdcr->GetInterimLeafByName(Kplus_py),
        _rdcr->GetInterimLeafByName(Kplus_pz),
        938.272046,
        _rdcr->GetInterimLeafByName(piminus_px),
        _rdcr->GetInterimLeafByName(piminus_py),
        _rdcr->GetInterimLeafByName(piminus_pz),
        493.677,
        _rdcr->GetInterimLeafByName(piminus0_px),
        _rdcr->GetInterimLeafByName(piminus0_py),
        _rdcr->GetInterimLeafByName(piminus0_pz),
        139.57018);
    _rdcr->RegisterDoubleLeaf(varDminusMassHypo_pKpi);

    KinematicReducerLeaf<Double_t>* varDplusMassHypo_ppiK = new KinematicReducerLeaf<Double_t>("varDplusMassHypo_ppiK", "varDplusMassHypo_ppiK", "Double_t", NULL);
    varDplusMassHypo_ppiK->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kminus_px),
        _rdcr->GetInterimLeafByName(Kminus_py),
        _rdcr->GetInterimLeafByName(Kminus_pz),
        938.272046,
        _rdcr->GetInterimLeafByName(piplus_px),
        _rdcr->GetInterimLeafByName(piplus_py),
        _rdcr->GetInterimLeafByName(piplus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piplus0_px),
        _rdcr->GetInterimLeafByName(piplus0_py),
        _rdcr->GetInterimLeafByName(piplus0_pz),
        493.677);
    _rdcr->RegisterDoubleLeaf(varDplusMassHypo_ppiK);

    KinematicReducerLeaf<Double_t>* varDminusMassHypo_ppiK = new KinematicReducerLeaf<Double_t>("varDminusMassHypo_ppiK", "varDminusMassHypo_ppiK", "Double_t", NULL);
    varDminusMassHypo_ppiK->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kplus_px),
        _rdcr->GetInterimLeafByName(Kplus_py),
        _rdcr->GetInterimLeafByName(Kplus_pz),
        938.272046,
        _rdcr->GetInterimLeafByName(piminus_px),
        _rdcr->GetInterimLeafByName(piminus_py),
        _rdcr->GetInterimLeafByName(piminus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piminus0_px),
        _rdcr->GetInterimLeafByName(piminus0_py),
        _rdcr->GetInterimLeafByName(piminus0_pz),
        493.677);
    _rdcr->RegisterDoubleLeaf(varDminusMassHypo_ppiK);

    KinematicReducerLeaf<Double_t>* varDplusMassHypo_pipK = new KinematicReducerLeaf<Double_t>("varDplusMassHypo_pipK", "varDplusMassHypo_pipK", "Double_t", NULL);
    varDplusMassHypo_pipK->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kminus_px),
        _rdcr->GetInterimLeafByName(Kminus_py),
        _rdcr->GetInterimLeafByName(Kminus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piplus_px),
        _rdcr->GetInterimLeafByName(piplus_py),
        _rdcr->GetInterimLeafByName(piplus_pz),
        938.272046,
        _rdcr->GetInterimLeafByName(piplus0_px),
        _rdcr->GetInterimLeafByName(piplus0_py),
        _rdcr->GetInterimLeafByName(piplus0_pz),
        493.677);
    _rdcr->RegisterDoubleLeaf(varDplusMassHypo_pipK);

    KinematicReducerLeaf<Double_t>* varDminusMassHypo_pipK = new KinematicReducerLeaf<Double_t>("varDminusMassHypo_pipK", "varDminusMassHypo_pipK", "Double_t", NULL);
    varDminusMassHypo_pipK->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kplus_px),
        _rdcr->GetInterimLeafByName(Kplus_py),
        _rdcr->GetInterimLeafByName(Kplus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piminus_px),
        _rdcr->GetInterimLeafByName(piminus_py),
        _rdcr->GetInterimLeafByName(piminus_pz),
        938.272046,
        _rdcr->GetInterimLeafByName(piminus0_px),
        _rdcr->GetInterimLeafByName(piminus0_py),
        _rdcr->GetInterimLeafByName(piminus0_pz),
        493.677);
    _rdcr->RegisterDoubleLeaf(varDminusMassHypo_pipK);

    KinematicReducerLeaf<Double_t>* varDplusMassHypo_piKp = new KinematicReducerLeaf<Double_t>("varDplusMassHypo_piKp", "varDplusMassHypo_piKp", "Double_t", NULL);
    varDplusMassHypo_piKp->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kminus_px),
        _rdcr->GetInterimLeafByName(Kminus_py),
        _rdcr->GetInterimLeafByName(Kminus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piplus_px),
        _rdcr->GetInterimLeafByName(piplus_py),
        _rdcr->GetInterimLeafByName(piplus_pz),
        493.677,
        _rdcr->GetInterimLeafByName(piplus0_px),
        _rdcr->GetInterimLeafByName(piplus0_py),
        _rdcr->GetInterimLeafByName(piplus0_pz),
        938.272046);
    _rdcr->RegisterDoubleLeaf(varDplusMassHypo_piKp);

    KinematicReducerLeaf<Double_t>* varDminusMassHypo_piKp = new KinematicReducerLeaf<Double_t>("varDminusMassHypo_piKp", "varDminusMassHypo_piKp", "Double_t", NULL);
    varDminusMassHypo_piKp->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kplus_px),
        _rdcr->GetInterimLeafByName(Kplus_py),
        _rdcr->GetInterimLeafByName(Kplus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piminus_px),
        _rdcr->GetInterimLeafByName(piminus_py),
        _rdcr->GetInterimLeafByName(piminus_pz),
        493.677,
        _rdcr->GetInterimLeafByName(piminus0_px),
        _rdcr->GetInterimLeafByName(piminus0_py),
        _rdcr->GetInterimLeafByName(piminus0_pz),
        938.272046);
    _rdcr->RegisterDoubleLeaf(varDminusMassHypo_piKp);

    KinematicReducerLeaf<Double_t>* varDplusMassHypo_ppipi = new KinematicReducerLeaf<Double_t>("varDplusMassHypo_ppipi", "varDplusMassHypo_ppipi", "Double_t", NULL);
    varDplusMassHypo_ppipi->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kminus_px),
        _rdcr->GetInterimLeafByName(Kminus_py),
        _rdcr->GetInterimLeafByName(Kminus_pz),
        938.272046,
        _rdcr->GetInterimLeafByName(piplus_px),
        _rdcr->GetInterimLeafByName(piplus_py),
        _rdcr->GetInterimLeafByName(piplus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piplus0_px),
        _rdcr->GetInterimLeafByName(piplus0_py),
        _rdcr->GetInterimLeafByName(piplus0_pz),
        139.57018);
    _rdcr->RegisterDoubleLeaf(varDplusMassHypo_ppipi);

    KinematicReducerLeaf<Double_t>* varDminusMassHypo_ppipi = new KinematicReducerLeaf<Double_t>("varDminusMassHypo_ppipi", "varDminusMassHypo_ppipi", "Double_t", NULL);
    varDminusMassHypo_ppipi->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kplus_px),
        _rdcr->GetInterimLeafByName(Kplus_py),
        _rdcr->GetInterimLeafByName(Kplus_pz),
        938.272046,
        _rdcr->GetInterimLeafByName(piminus_px),
        _rdcr->GetInterimLeafByName(piminus_py),
        _rdcr->GetInterimLeafByName(piminus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piminus0_px),
        _rdcr->GetInterimLeafByName(piminus0_py),
        _rdcr->GetInterimLeafByName(piminus0_pz),
        139.57018);
    _rdcr->RegisterDoubleLeaf(varDminusMassHypo_ppipi);

    KinematicReducerLeaf<Double_t>* varDplusMassHypo_pippi = new KinematicReducerLeaf<Double_t>("varDplusMassHypo_pippi", "varDplusMassHypo_pippi", "Double_t", NULL);
    varDplusMassHypo_pippi->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kminus_px),
        _rdcr->GetInterimLeafByName(Kminus_py),
        _rdcr->GetInterimLeafByName(Kminus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piplus_px),
        _rdcr->GetInterimLeafByName(piplus_py),
        _rdcr->GetInterimLeafByName(piplus_pz),
        938.272046,
        _rdcr->GetInterimLeafByName(piplus0_px),
        _rdcr->GetInterimLeafByName(piplus0_py),
        _rdcr->GetInterimLeafByName(piplus0_pz),
        139.57018);
    _rdcr->RegisterDoubleLeaf(varDplusMassHypo_pippi);

    KinematicReducerLeaf<Double_t>* varDminusMassHypo_pippi = new KinematicReducerLeaf<Double_t>("varDminusMassHypo_pippi", "varDminusMassHypo_pippi", "Double_t", NULL);
    varDminusMassHypo_pippi->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kplus_px),
        _rdcr->GetInterimLeafByName(Kplus_py),
        _rdcr->GetInterimLeafByName(Kplus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piminus_px),
        _rdcr->GetInterimLeafByName(piminus_py),
        _rdcr->GetInterimLeafByName(piminus_pz),
        938.272046,
        _rdcr->GetInterimLeafByName(piminus0_px),
        _rdcr->GetInterimLeafByName(piminus0_py),
        _rdcr->GetInterimLeafByName(piminus0_pz),
        139.57018);
    _rdcr->RegisterDoubleLeaf(varDminusMassHypo_pippi);

    KinematicReducerLeaf<Double_t>* varDplusMassHypo_pipip = new KinematicReducerLeaf<Double_t>("varDplusMassHypo_pipip", "varDplusMassHypo_pipip", "Double_t", NULL);
    varDplusMassHypo_pipip->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kminus_px),
        _rdcr->GetInterimLeafByName(Kminus_py),
        _rdcr->GetInterimLeafByName(Kminus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piplus_px),
        _rdcr->GetInterimLeafByName(piplus_py),
        _rdcr->GetInterimLeafByName(piplus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piplus0_px),
        _rdcr->GetInterimLeafByName(piplus0_py),
        _rdcr->GetInterimLeafByName(piplus0_pz),
        938.272046);
    _rdcr->RegisterDoubleLeaf(varDplusMassHypo_pipip);

    KinematicReducerLeaf<Double_t>* varDminusMassHypo_pipip = new KinematicReducerLeaf<Double_t>("varDminusMassHypo_pipip", "varDminusMassHypo_pipip", "Double_t", NULL);
    varDminusMassHypo_pipip->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Kplus_px),
        _rdcr->GetInterimLeafByName(Kplus_py),
        _rdcr->GetInterimLeafByName(Kplus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piminus_px),
        _rdcr->GetInterimLeafByName(piminus_py),
        _rdcr->GetInterimLeafByName(piminus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piminus0_px),
        _rdcr->GetInterimLeafByName(piminus0_py),
        _rdcr->GetInterimLeafByName(piminus0_pz),
        938.272046);
    _rdcr->RegisterDoubleLeaf(varDminusMassHypo_pipip);

    KinematicReducerLeaf<Double_t>* varBMassHypo_Dpluspipi = new KinematicReducerLeaf<Double_t>("varBMassHypo_Dpluspipi", "varBMassHypo_Dpluspipi", "Double_t", NULL);
    varBMassHypo_Dpluspipi->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Dplus_px),
        _rdcr->GetInterimLeafByName(Dplus_py),
        _rdcr->GetInterimLeafByName(Dplus_pz),
        1869.62,
        _rdcr->GetInterimLeafByName(piminus_px),
        _rdcr->GetInterimLeafByName(piminus_py),
        _rdcr->GetInterimLeafByName(piminus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piminus0_px),
        _rdcr->GetInterimLeafByName(piminus0_py),
        _rdcr->GetInterimLeafByName(piminus0_pz),
        139.57018);
    _rdcr->RegisterDoubleLeaf(varBMassHypo_Dpluspipi);

    KinematicReducerLeaf<Double_t>* varBMassHypo_Dminuspipi = new KinematicReducerLeaf<Double_t>("varBMassHypo_Dminuspipi", "varBMassHypo_Dminuspipi", "Double_t", NULL);
    varBMassHypo_Dminuspipi->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Dminus_px),
        _rdcr->GetInterimLeafByName(Dminus_py),
        _rdcr->GetInterimLeafByName(Dminus_pz),
        1869.62,
        _rdcr->GetInterimLeafByName(piplus_px),
        _rdcr->GetInterimLeafByName(piplus_py),
        _rdcr->GetInterimLeafByName(piplus_pz),
        139.57018,
        _rdcr->GetInterimLeafByName(piplus0_px),
        _rdcr->GetInterimLeafByName(piplus0_py),
        _rdcr->GetInterimLeafByName(piplus0_pz),
        139.57018);
    _rdcr->RegisterDoubleLeaf(varBMassHypo_Dminuspipi);

    KinematicReducerLeaf<Double_t>* varBMassHypo_DplusKpi = new KinematicReducerLeaf<Double_t>("varBMassHypo_DplusKpi", "varBMassHypo_DplusKpi", "Double_t", NULL);
    varBMassHypo_DplusKpi->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Dplus_px),
        _rdcr->GetInterimLeafByName(Dplus_py),
        _rdcr->GetInterimLeafByName(Dplus_pz),
        1869.62,
        _rdcr->GetInterimLeafByName(Kplus_px),
        _rdcr->GetInterimLeafByName(Kplus_py),
        _rdcr->GetInterimLeafByName(Kplus_pz),
        493.677,
        _rdcr->GetInterimLeafByName(piminus_px),
        _rdcr->GetInterimLeafByName(piminus_py),
        _rdcr->GetInterimLeafByName(piminus_pz),
        139.57018);
    _rdcr->RegisterDoubleLeaf(varBMassHypo_DplusKpi);

    KinematicReducerLeaf<Double_t>* varBMassHypo_DminusKpi = new KinematicReducerLeaf<Double_t>("varBMassHypo_DminusKpi", "varBMassHypo_DminusKpi", "Double_t", NULL);
    varBMassHypo_DminusKpi->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Dminus_px),
        _rdcr->GetInterimLeafByName(Dminus_py),
        _rdcr->GetInterimLeafByName(Dminus_pz),
        1869.62,
        _rdcr->GetInterimLeafByName(Kminus_px),
        _rdcr->GetInterimLeafByName(Kminus_py),
        _rdcr->GetInterimLeafByName(Kminus_pz),
        493.677,
        _rdcr->GetInterimLeafByName(piplus_px),
        _rdcr->GetInterimLeafByName(piplus_py),
        _rdcr->GetInterimLeafByName(piplus_pz),
        139.57018);
    _rdcr->RegisterDoubleLeaf(varBMassHypo_DminusKpi);

    KinematicReducerLeaf<Double_t>* varBMassHypo_DpluspiK = new KinematicReducerLeaf<Double_t>("varBMassHypo_DpluspiK", "varBMassHypo_DpluspiK", "Double_t", NULL);
    varBMassHypo_DpluspiK->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Dplus_px),
        _rdcr->GetInterimLeafByName(Dplus_py),
        _rdcr->GetInterimLeafByName(Dplus_pz),
        1869.62,
        _rdcr->GetInterimLeafByName(Kplus_px),
        _rdcr->GetInterimLeafByName(Kplus_py),
        _rdcr->GetInterimLeafByName(Kplus_pz),
        493.677,
        _rdcr->GetInterimLeafByName(piminus0_px),
        _rdcr->GetInterimLeafByName(piminus0_py),
        _rdcr->GetInterimLeafByName(piminus0_pz),
        139.57018);
    _rdcr->RegisterDoubleLeaf(varBMassHypo_DpluspiK);

    KinematicReducerLeaf<Double_t>* varBMassHypo_DminuspiK = new KinematicReducerLeaf<Double_t>("varBMassHypo_DminuspiK", "varBMassHypo_DminuspiK", "Double_t", NULL);
    varBMassHypo_DminuspiK->FixedMassDaughtersThreeBodyDecayMotherMass(
        _rdcr->GetInterimLeafByName(Dminus_px),
        _rdcr->GetInterimLeafByName(Dminus_py),
        _rdcr->GetInterimLeafByName(Dminus_pz),
        1869.62,
        _rdcr->GetInterimLeafByName(Kminus_px),
        _rdcr->GetInterimLeafByName(Kminus_py),
        _rdcr->GetInterimLeafByName(Kminus_pz),
        493.677,
        _rdcr->GetInterimLeafByName(piplus0_px),
        _rdcr->GetInterimLeafByName(piplus0_py),
        _rdcr->GetInterimLeafByName(piplus0_pz),
        139.57018);
    _rdcr->RegisterDoubleLeaf(varBMassHypo_DminuspiK);

    doocore::io::sinfo << "Veto leaves are filled using constrain: " << mass_hypo_constraints << doocore::io::endmsg;
  }
  summary.Add("Veto fit constraints", mass_hypo_constraints);
}

void AuxiliaryLeaves(Reducer* _rdcr, cfg_tuple& cfg){
  doocore::config::Summary& summary = doocore::config::Summary::GetInstance();
  // handle flattened tuples
  std::string flat_suffix = "";
  if (std::get<4>(cfg)) flat_suffix = "_flat";

  // random leaf
  TRandom3* random_generator_ = new TRandom3(42);
  ReducerLeaf<Int_t>& random_leaf = _rdcr->CreateIntLeaf("idxRandom", -1);
  random_leaf.Randomize(random_generator_);

  // background category
  if (std::get<3>(cfg)){
    _rdcr->CreateIntCopyLeaf("catBkg", _rdcr->GetInterimLeafByName("B0_BKGCAT"));
  }
  // final state
  ReducerLeaf<Int_t>& catDplusFinalState = _rdcr->CreateIntLeaf("catDplusFinalState", -10);
      catDplusFinalState.AddCondition("Kpipi","Dplus_Kminus_or_piminus_ID==-321&&Dplus_piplus_or_Kplus_One_ID==211&&Dplus_piplus_or_Kplus_Two_ID==211",1);
      catDplusFinalState.AddCondition("pipipi","Dplus_Kminus_or_piminus_ID==-211&&Dplus_piplus_or_Kplus_One_ID==211&&Dplus_piplus_or_Kplus_Two_ID==211",2);
      catDplusFinalState.AddCondition("KKpi","Dplus_Kminus_or_piminus_ID==-321&&((Dplus_piplus_or_Kplus_One_ID==321&&Dplus_piplus_or_Kplus_Two_ID==211)||(Dplus_piplus_or_Kplus_One_ID==211&&Dplus_piplus_or_Kplus_Two_ID==321))",3);

  ReducerLeaf<Int_t>& catDminusFinalState = _rdcr->CreateIntLeaf("catDminusFinalState", -10);
      catDminusFinalState.AddCondition("Kpipi","Dminus_Kplus_or_piplus_ID==321&&Dminus_piminus_or_Kminus_One_ID==-211&&Dminus_piminus_or_Kminus_Two_ID==-211",1);
      catDminusFinalState.AddCondition("pipipi","Dminus_Kplus_or_piplus_ID==211&&Dminus_piminus_or_Kminus_One_ID==-211&&Dminus_piminus_or_Kminus_Two_ID==-211",2);
      catDminusFinalState.AddCondition("KKpi","Dminus_Kplus_or_piplus_ID==321&&((Dminus_piminus_or_Kminus_One_ID==-321&&Dminus_piminus_or_Kminus_Two_ID==-211)||(Dminus_piminus_or_Kminus_One_ID==-211&&Dminus_piminus_or_Kminus_Two_ID==-321))",3);

  ReducerLeaf<Int_t>& catDDFinalState = _rdcr->CreateIntLeaf("catDDFinalState", -10);
      catDDFinalState.AddCondition("KpipiKpipi", "catDplusFinalState==1&&catDminusFinalState==1", 1);
      catDDFinalState.AddCondition("Kpipipipipi", "catDplusFinalState==1&&catDminusFinalState==2", 2);
      catDDFinalState.AddCondition("pipipiKpipi", "catDplusFinalState==2&&catDminusFinalState==1", 3);
      catDDFinalState.AddCondition("KpipiKKpi", "catDplusFinalState==1&&catDminusFinalState==3", 4);
      catDDFinalState.AddCondition("KKpiKpipi", "catDplusFinalState==3&&catDminusFinalState==1", 5);
      catDDFinalState.AddCondition("pipipipipipi", "catDplusFinalState==2&&catDminusFinalState==2", 6);
      catDDFinalState.AddCondition("pipipiKKpi", "catDplusFinalState==2&&catDminusFinalState==3", 7);
      catDDFinalState.AddCondition("KKpipipipi", "catDplusFinalState==3&&catDminusFinalState==2", 8);
      catDDFinalState.AddCondition("KKpiKKpi", "catDplusFinalState==3&&catDminusFinalState==3", 9);

  // event and run number
  _rdcr->CreateIntCopyLeaf("idxEventNumber", _rdcr->GetInterimLeafByName("eventNumber"));
  _rdcr->CreateIntCopyLeaf("idxRunNumber", _rdcr->GetInterimLeafByName("runNumber"));
  // number of PVs
  _rdcr->CreateIntCopyLeaf("catNPV", _rdcr->GetInterimLeafByName("nPV"));
  // magnet direction
  _rdcr->CreateIntCopyLeaf("catMag", _rdcr->GetInterimLeafByName("Polarity"));
  // number of tracks
  _rdcr->CreateIntCopyLeaf("catNTrack", _rdcr->GetInterimLeafByName("nTracks"));
  // track ghost probability
  _rdcr->CreateDoubleCopyLeaf("varKplusTrackGhostProb", _rdcr->GetInterimLeafByName("Dminus_Kplus_or_piplus_TRACK_GhostProb"));
  _rdcr->CreateDoubleCopyLeaf("varKminusTrackGhostProb", _rdcr->GetInterimLeafByName("Dplus_Kminus_or_piminus_TRACK_GhostProb"));
  _rdcr->CreateDoubleLeaf("varKaonMaxTrackGhostProb", -999999.).Maximum(_rdcr->GetInterimLeafByName("Dminus_Kplus_or_piplus_TRACK_GhostProb"), _rdcr->GetInterimLeafByName("Dplus_Kminus_or_piminus_TRACK_GhostProb"));
  _rdcr->CreateDoubleCopyLeaf("varPiplusOneTrackGhostProb", _rdcr->GetInterimLeafByName("Dplus_piplus_or_Kplus_One_TRACK_GhostProb"));
  _rdcr->CreateDoubleCopyLeaf("varPiplusTwoTrackGhostProb", _rdcr->GetInterimLeafByName("Dplus_piplus_or_Kplus_Two_TRACK_GhostProb"));
  ReducerLeaf<Double_t>& varPiplusMaxTrackGhostProb = _rdcr->CreateDoubleLeaf("varPiplusMaxTrackGhostProb", -999999.);
  varPiplusMaxTrackGhostProb.Maximum(_rdcr->GetInterimLeafByName("Dplus_piplus_or_Kplus_One_TRACK_GhostProb"), _rdcr->GetInterimLeafByName("Dplus_piplus_or_Kplus_Two_TRACK_GhostProb"));
  _rdcr->CreateDoubleCopyLeaf("varPiminusOneTrackGhostProb", _rdcr->GetInterimLeafByName("Dminus_piminus_or_Kminus_One_TRACK_GhostProb"));
  _rdcr->CreateDoubleCopyLeaf("varPiminusTwoTrackGhostProb", _rdcr->GetInterimLeafByName("Dminus_piminus_or_Kminus_Two_TRACK_GhostProb"));
  ReducerLeaf<Double_t>& varPiminusMaxTrackGhostProb = _rdcr->CreateDoubleLeaf("varPiminusMaxTrackGhostProb", -999999.);
  varPiminusMaxTrackGhostProb.Maximum(_rdcr->GetInterimLeafByName("Dminus_piminus_or_Kminus_One_TRACK_GhostProb"), _rdcr->GetInterimLeafByName("Dminus_piminus_or_Kminus_Two_TRACK_GhostProb"));
  _rdcr->CreateDoubleLeaf("varPionMaxTrackGhostProb", -999999.).Maximum(varPiplusMaxTrackGhostProb, varPiminusMaxTrackGhostProb);

  // End vertex chi2/ndof
  _rdcr->CreateDoubleLeaf("varBEndVtxChi2ndof", -99999999.).Divide(_rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_ENDVERTEX_CHI2"), _rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_ENDVERTEX_NDOF"));
  ReducerLeaf<Double_t>& varDplusEndVtxChi2ndof = _rdcr->CreateDoubleLeaf("varDplusEndVtxChi2ndof", -99999999.);
  varDplusEndVtxChi2ndof.Divide(_rdcr->GetInterimLeafByName("Dplus_ENDVERTEX_CHI2"), _rdcr->GetInterimLeafByName("Dplus_ENDVERTEX_NDOF"));
  ReducerLeaf<Double_t>& varDminusEndVtxChi2ndof = _rdcr->CreateDoubleLeaf("varDminusEndVtxChi2ndof", -99999999.);
  varDminusEndVtxChi2ndof.Divide(_rdcr->GetInterimLeafByName("Dminus_ENDVERTEX_CHI2"), _rdcr->GetInterimLeafByName("Dminus_ENDVERTEX_NDOF"));
  _rdcr->CreateDoubleLeaf("varDMaxEndVtxChi2ndof", -999999.).Maximum(varDplusEndVtxChi2ndof, varDminusEndVtxChi2ndof);
  // sum of D transverse momenta
  _rdcr->CreateDoubleLeaf("varDDDTFSumPT", -99999999.).Add(_rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_FitPVConst_Dplus_PT"+flat_suffix), _rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_FitPVConst_Dminus_PT"+flat_suffix));
  // maximal kaon track fit chi2ndof
  _rdcr->CreateDoubleLeaf("varKaonMaxTrackFitChi2ndof", -999999.).Maximum(_rdcr->GetInterimLeafByName("Dminus_Kplus_or_piplus_TRACK_CHI2NDOF"), _rdcr->GetInterimLeafByName("Dplus_Kminus_or_piminus_TRACK_CHI2NDOF"));
  // maximal pion track fit chi2ndof
  ReducerLeaf<Double_t>& piplus_max_trackchi2 = _rdcr->CreateDoubleLeaf("varPiPlusMaxTrackFitChi2ndof", -999999.);
  piplus_max_trackchi2.Maximum(_rdcr->GetInterimLeafByName("Dplus_piplus_or_Kplus_One_TRACK_CHI2NDOF"), _rdcr->GetInterimLeafByName("Dplus_piplus_or_Kplus_Two_TRACK_CHI2NDOF"));
  ReducerLeaf<Double_t>& piminus_max_trackchi2 = _rdcr->CreateDoubleLeaf("varPiMinusMaxTrackFitChi2ndof", -999999.);
  piminus_max_trackchi2.Maximum(_rdcr->GetInterimLeafByName("Dminus_piminus_or_Kminus_One_TRACK_CHI2NDOF"), _rdcr->GetInterimLeafByName("Dminus_piminus_or_Kminus_Two_TRACK_CHI2NDOF"));
  _rdcr->CreateDoubleLeaf("varPionMaxTrackFitChi2ndof", -999999.).Maximum(piplus_max_trackchi2, piminus_max_trackchi2);
  // minimal kaon MinIP chi2
  _rdcr->CreateDoubleLeaf("varKaonMinMinIPChi2", -999999.).Minimum(_rdcr->GetInterimLeafByName("Dminus_Kplus_or_piplus_MINIPCHI2"), _rdcr->GetInterimLeafByName("Dplus_Kminus_or_piminus_MINIPCHI2"));
  // minimal pion MinIP chi2
  ReducerLeaf<Double_t>& piplus_min_minipchi2 = _rdcr->CreateDoubleLeaf("varPiplusMinMinIPChi2", -999999.);
  piplus_min_minipchi2.Minimum(_rdcr->GetInterimLeafByName("Dplus_piplus_or_Kplus_One_MINIPCHI2"), _rdcr->GetInterimLeafByName("Dplus_piplus_or_Kplus_Two_MINIPCHI2"));
  ReducerLeaf<Double_t>& piminus_min_minipchi2 = _rdcr->CreateDoubleLeaf("varPiminusMinMinIPChi2", -999999.);
  piminus_min_minipchi2.Minimum(_rdcr->GetInterimLeafByName("Dminus_piminus_or_Kminus_One_MINIPCHI2"), _rdcr->GetInterimLeafByName("Dminus_piminus_or_Kminus_Two_MINIPCHI2"));
  _rdcr->CreateDoubleLeaf("varPionMinMinIPChi2", -999999.).Minimum(piplus_min_minipchi2, piminus_min_minipchi2);
  // sum of D daughters transverse momentum
  ReducerLeaf<Double_t>& dplus_sum_daughters_pt = _rdcr->CreateDoubleLeaf("varDplusDaughtersSumPT", -999999.);
  dplus_sum_daughters_pt.Add(_rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_FitPVConst_Dplus_P0_PT"+flat_suffix), _rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_FitPVConst_Dplus_P1_PT"+flat_suffix));
  dplus_sum_daughters_pt.Add(dplus_sum_daughters_pt, _rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_FitPVConst_Dplus_P2_PT"+flat_suffix));
  ReducerLeaf<Double_t>& dminus_sum_daughters_pt = _rdcr->CreateDoubleLeaf("varDminusDaughtersSumPT", -999999.);
  dminus_sum_daughters_pt.Add(_rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_FitPVConst_Dminus_P0_PT"+flat_suffix), _rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_FitPVConst_Dminus_P1_PT"+flat_suffix));
  dminus_sum_daughters_pt.Add(dminus_sum_daughters_pt, _rdcr->GetInterimLeafByName(std::get<0>(cfg)+"_FitPVConst_Dminus_P2_PT"+flat_suffix));
  _rdcr->CreateDoubleLeaf("varDDaughtersMinSumPT", -999999.).Minimum(dminus_sum_daughters_pt, dplus_sum_daughters_pt);
  _rdcr->CreateDoubleLeaf("varDDaughtersMaxSumPT", -999999.).Maximum(dminus_sum_daughters_pt, dplus_sum_daughters_pt);

  // data taking period
  ReducerLeaf<Int_t>& cat_year_leaf = _rdcr->CreateIntLeaf("catYear", 0);
    cat_year_leaf.AddCondition("2011", "GpsTime < 1.325376e+15",  2011);
    cat_year_leaf.AddCondition("2012", "GpsTime >= 1.325376e+15", 2012);

  // decay tree fit
  // fit status
  if (_rdcr->LeafExists("B0_FitPVConst_status"+flat_suffix)) _rdcr->CreateIntCopyLeaf("varDTFStatusPVConst", _rdcr->GetInterimLeafByName("B0_FitPVConst_status"+flat_suffix));
  if (_rdcr->LeafExists("B0_FitDDPVConst_status"+flat_suffix)) _rdcr->CreateIntCopyLeaf("varDTFStatusDDPVConst", _rdcr->GetInterimLeafByName("B0_FitDDPVConst_status"+flat_suffix));

  // chi2ndof
  ReducerLeaf<Double_t>* dtf_chi2ndof_leaf_ptr = NULL;
  if (_rdcr->LeafExists("B0_FitPVConst_chi2"+flat_suffix)) {
    dtf_chi2ndof_leaf_ptr = &_rdcr->CreateDoubleLeaf("varDTFChi2ndof", -1.0);
    dtf_chi2ndof_leaf_ptr->Divide(_rdcr->GetInterimLeafByName("B0_FitPVConst_chi2"+flat_suffix),
                                  _rdcr->GetInterimLeafByName("B0_FitPVConst_nDOF"+flat_suffix));
  } else if (_rdcr->LeafExists("B0_LOKI_DTF_CHI2NDOF")) {
    dtf_chi2ndof_leaf_ptr = &_rdcr->CreateDoubleCopyLeaf("varDTFChi2ndof", _rdcr->GetInterimLeafByName("B0_LOKI_DTF_CHI2NDOF"));
  }

  // IP chi2
  ReducerLeaf<Double_t>* ip_chi2_leaf_ptr = NULL;
  if (_rdcr->LeafExists("B0_FitPVConst_IPCHI2"+flat_suffix)) {
    ip_chi2_leaf_ptr = &_rdcr->CreateDoubleCopyLeaf("varDTFIPChi2", _rdcr->GetInterimLeafByName("B0_FitPVConst_IPCHI2"+flat_suffix));
  } else if (_rdcr->LeafExists("B0_IPCHI2_OWNPV")) {
    ip_chi2_leaf_ptr = &_rdcr->CreateDoubleCopyLeaf("varIPChi2OwnPV", _rdcr->GetInterimLeafByName("B0_IPCHI2_OWNPV"));
  }
  ReducerLeaf<Double_t>* dplus_ip_chi2_leaf_ptr = NULL;
  ReducerLeaf<Double_t>* dminus_ip_chi2_leaf_ptr = NULL;
  if (_rdcr->LeafExists("B0_FitPVConst_Dplus_IPCHI2"+flat_suffix)) {
    dplus_ip_chi2_leaf_ptr = &_rdcr->CreateDoubleCopyLeaf("varDplusIPChi2", _rdcr->GetInterimLeafByName("B0_FitPVConst_Dplus_IPCHI2"+flat_suffix));
    dminus_ip_chi2_leaf_ptr = &_rdcr->CreateDoubleCopyLeaf("varDminusIPChi2", _rdcr->GetInterimLeafByName("B0_FitPVConst_Dminus_IPCHI2"+flat_suffix));
  } else if (_rdcr->LeafExists("Dplus_IPCHI2_OWNPV")) {
    dplus_ip_chi2_leaf_ptr = &_rdcr->CreateDoubleCopyLeaf("varDplusIPChi2", _rdcr->GetInterimLeafByName("Dplus_IPCHI2_OWNPV"));
    dminus_ip_chi2_leaf_ptr = &_rdcr->CreateDoubleCopyLeaf("varDminusIPChi2", _rdcr->GetInterimLeafByName("Dminus_IPCHI2_OWNPV"));
  }
  _rdcr->CreateDoubleLeaf("varDIPChi2Minimum", -999999.).Minimum(*dplus_ip_chi2_leaf_ptr, *dminus_ip_chi2_leaf_ptr);

  // Direction angle (DIRA)
  ReducerLeaf<Double_t>* dira_leaf_ptr = NULL;
  ReducerLeaf<Double_t>* dplus_dira_leaf_ptr = NULL;
  ReducerLeaf<Double_t>* dminus_dira_leaf_ptr = NULL;
  if (_rdcr->LeafExists("B0_FitPVConst_DIRA"+flat_suffix)) {
    dira_leaf_ptr = &_rdcr->CreateDoubleCopyLeaf("varBDTFDIRA", _rdcr->GetInterimLeafByName("B0_FitPVConst_DIRA"+flat_suffix));
    dplus_dira_leaf_ptr = &_rdcr->CreateDoubleCopyLeaf("varDplusDTFDIRA", _rdcr->GetInterimLeafByName("B0_FitPVConst_Dplus_DIRA"+flat_suffix));
    dminus_dira_leaf_ptr = &_rdcr->CreateDoubleCopyLeaf("varDminusDTFDIRA", _rdcr->GetInterimLeafByName("B0_FitPVConst_Dminus_DIRA"+flat_suffix));
  } else if (_rdcr->LeafExists("B0_DIRA_OWNPV")) {
    dira_leaf_ptr = &_rdcr->CreateDoubleCopyLeaf("varDIRAOwnPV", _rdcr->GetInterimLeafByName("B0_DIRA_OWNPV"));
    dplus_dira_leaf_ptr = &_rdcr->CreateDoubleCopyLeaf("varDplusDIRAOwnPV", _rdcr->GetInterimLeafByName("Dplus_DIRA_OWNPV"));
    dminus_dira_leaf_ptr = &_rdcr->CreateDoubleCopyLeaf("varDminusDIRAOwnPV", _rdcr->GetInterimLeafByName("Dminus_DIRA_OWNPV"));
  }
  _rdcr->CreateDoubleLeaf("varDMinDira", -999999.).Minimum(*dplus_dira_leaf_ptr, *dminus_dira_leaf_ptr);

  // DTF PV position
  if (_rdcr->LeafExists("B0_FitPVConst_PV_X"+flat_suffix)) _rdcr->CreateDoubleCopyLeaf("varDTFPVPosX", _rdcr->GetInterimLeafByName("B0_FitPVConst_PV_X"+flat_suffix));
  if (_rdcr->LeafExists("B0_FitPVConst_PV_Y"+flat_suffix)) _rdcr->CreateDoubleCopyLeaf("varDTFPVPosY", _rdcr->GetInterimLeafByName("B0_FitPVConst_PV_Y"+flat_suffix));
  if (_rdcr->LeafExists("B0_FitPVConst_PV_Z"+flat_suffix)) _rdcr->CreateDoubleCopyLeaf("varDTFPVPosZ", _rdcr->GetInterimLeafByName("B0_FitPVConst_PV_Z"+flat_suffix));

  // momentum leaves
  // standard leaves
  _rdcr->CreateDoubleCopyLeaf("obsMomentum", _rdcr->GetInterimLeafByName("B0_P"));
  _rdcr->CreateDoubleCopyLeaf("obsMomentumX", _rdcr->GetInterimLeafByName("B0_PX"));
  _rdcr->CreateDoubleCopyLeaf("obsMomentumY", _rdcr->GetInterimLeafByName("B0_PY"));
  _rdcr->CreateDoubleCopyLeaf("obsMomentumZ", _rdcr->GetInterimLeafByName("B0_PZ"));
  _rdcr->CreateDoubleCopyLeaf("obsTransverseMomentum", _rdcr->GetInterimLeafByName("B0_PT"));
  // DTF leaves
  if (_rdcr->LeafExists("B0_FitPVConst_P"+flat_suffix)){
    _rdcr->CreateDoubleCopyLeaf("obsDTFMomentum", _rdcr->GetInterimLeafByName("B0_FitPVConst_P"+flat_suffix));
    _rdcr->CreateDoubleCopyLeaf("obsDTFMomentumError", _rdcr->GetInterimLeafByName("B0_FitPVConst_PERR"+flat_suffix));
    _rdcr->CreateDoubleCopyLeaf("obsDTFTransverseMomentum", _rdcr->GetInterimLeafByName("B0_FitPVConst_PT"+flat_suffix));
    _rdcr->CreateDoubleCopyLeaf("obsDTFTransverseMomentumError", _rdcr->GetInterimLeafByName("B0_FitPVConst_PTERR"+flat_suffix));
  }
  // daughters
  _rdcr->CreateDoubleCopyLeaf("varDplusP", _rdcr->GetInterimLeafByName("B0_FitPVConst_Dplus_P"+flat_suffix));
  _rdcr->CreateDoubleCopyLeaf("varDplusPT", _rdcr->GetInterimLeafByName("B0_FitPVConst_Dplus_PT"+flat_suffix));
  _rdcr->CreateDoubleCopyLeaf("varDminusP", _rdcr->GetInterimLeafByName("B0_FitPVConst_Dminus_P"+flat_suffix));
  _rdcr->CreateDoubleCopyLeaf("varDminusPT", _rdcr->GetInterimLeafByName("B0_FitPVConst_Dminus_PT"+flat_suffix));
  _rdcr->CreateDoubleLeaf("varDMinPT", -999999.).Minimum(_rdcr->GetInterimLeafByName("B0_FitPVConst_Dplus_PT"+flat_suffix), _rdcr->GetInterimLeafByName("B0_FitPVConst_Dminus_PT"+flat_suffix));
  // grand-daughters
  _rdcr->CreateDoubleCopyLeaf("varpiplusOneP", _rdcr->GetInterimLeafByName("B0_FitPVConst_Dminus_P0_P"+flat_suffix));
  _rdcr->CreateDoubleCopyLeaf("varpiplusOnePT", _rdcr->GetInterimLeafByName("B0_FitPVConst_Dminus_P0_PT"+flat_suffix));
  _rdcr->CreateDoubleCopyLeaf("varpiplusTwoP", _rdcr->GetInterimLeafByName("B0_FitPVConst_Dminus_P1_P"+flat_suffix));
  _rdcr->CreateDoubleCopyLeaf("varpiplusTwoPT", _rdcr->GetInterimLeafByName("B0_FitPVConst_Dminus_P1_PT"+flat_suffix));
  _rdcr->CreateDoubleCopyLeaf("varpiminusOneP", _rdcr->GetInterimLeafByName("B0_FitPVConst_Dplus_P0_P"+flat_suffix));
  _rdcr->CreateDoubleCopyLeaf("varpiminusOnePT", _rdcr->GetInterimLeafByName("B0_FitPVConst_Dplus_P0_PT"+flat_suffix));
  _rdcr->CreateDoubleCopyLeaf("varpiminusTwoP", _rdcr->GetInterimLeafByName("B0_FitPVConst_Dplus_P1_P"+flat_suffix));
  _rdcr->CreateDoubleCopyLeaf("varpiminusTwoPT", _rdcr->GetInterimLeafByName("B0_FitPVConst_Dplus_P1_PT"+flat_suffix));
  _rdcr->CreateDoubleCopyLeaf("varKplusP", _rdcr->GetInterimLeafByName("B0_FitPVConst_Dminus_P2_P"+flat_suffix));
  _rdcr->CreateDoubleCopyLeaf("varKplusPT", _rdcr->GetInterimLeafByName("B0_FitPVConst_Dminus_P2_PT"+flat_suffix));
  _rdcr->CreateDoubleCopyLeaf("varKminusP", _rdcr->GetInterimLeafByName("B0_FitPVConst_Dplus_P2_P"+flat_suffix));
  _rdcr->CreateDoubleCopyLeaf("varKminusPT", _rdcr->GetInterimLeafByName("B0_FitPVConst_Dplus_P2_PT"+flat_suffix));
  _rdcr->CreateDoubleLeaf("varPionOneMinPT", -999999.).Minimum(_rdcr->GetInterimLeafByName("B0_FitPVConst_Dminus_P0_PT"+flat_suffix), _rdcr->GetInterimLeafByName("B0_FitPVConst_Dplus_P0_PT"+flat_suffix));
  _rdcr->CreateDoubleLeaf("varPionTwoMinPT", -999999.).Minimum(_rdcr->GetInterimLeafByName("B0_FitPVConst_Dminus_P1_PT"+flat_suffix), _rdcr->GetInterimLeafByName("B0_FitPVConst_Dplus_P1_PT"+flat_suffix));
  _rdcr->CreateDoubleLeaf("varKaonMinPT", -999999.).Minimum(_rdcr->GetInterimLeafByName("B0_FitPVConst_Dminus_P2_PT"+flat_suffix), _rdcr->GetInterimLeafByName("B0_FitPVConst_Dplus_P2_PT"+flat_suffix));

  // Velo track quality
  _rdcr->CreateDoubleCopyLeaf("varKminusVELOChi2ndof", _rdcr->GetInterimLeafByName("Dplus_Kminus_or_piminus_TRACK_VeloCHI2NDOF"));
  _rdcr->CreateDoubleCopyLeaf("varKplusVELOChi2ndof", _rdcr->GetInterimLeafByName("Dminus_Kplus_or_piplus_TRACK_VeloCHI2NDOF"));
  _rdcr->CreateDoubleLeaf("varKaonMinVELOChi2ndof", -999999.).Minimum(_rdcr->GetInterimLeafByName("Dplus_Kminus_or_piminus_TRACK_VeloCHI2NDOF"), _rdcr->GetInterimLeafByName("Dminus_Kplus_or_piplus_TRACK_VeloCHI2NDOF"));
  ReducerLeaf<Double_t>& varPiplusOneVELOChi2ndof = _rdcr->CreateDoubleCopyLeaf("varPiplusOneVELOChi2ndof", _rdcr->GetInterimLeafByName("Dplus_piplus_or_Kplus_One_TRACK_VeloCHI2NDOF"));
  ReducerLeaf<Double_t>& varPiplusTwoVELOChi2ndof = _rdcr->CreateDoubleCopyLeaf("varPiplusTwoVELOChi2ndof", _rdcr->GetInterimLeafByName("Dplus_piplus_or_Kplus_Two_TRACK_VeloCHI2NDOF"));
  ReducerLeaf<Double_t>& varPiplusMinVELOChi2ndof = _rdcr->CreateDoubleLeaf("varPiplusMinVELOChi2ndof", -999999.);
  varPiplusMinVELOChi2ndof.Minimum(varPiplusOneVELOChi2ndof, varPiplusTwoVELOChi2ndof);
  ReducerLeaf<Double_t>& varPiminusOneVELOChi2ndof = _rdcr->CreateDoubleCopyLeaf("varPiminusOneVELOChi2ndof", _rdcr->GetInterimLeafByName("Dminus_piminus_or_Kminus_One_TRACK_VeloCHI2NDOF"));
  ReducerLeaf<Double_t>& varPiminusTwoVELOChi2ndof = _rdcr->CreateDoubleCopyLeaf("varPiminusTwoVELOChi2ndof", _rdcr->GetInterimLeafByName("Dminus_piminus_or_Kminus_Two_TRACK_VeloCHI2NDOF"));
  ReducerLeaf<Double_t>& varPiminusMinVELOChi2ndof = _rdcr->CreateDoubleLeaf("varPiminusMinVELOChi2ndof", -999999.);
  varPiminusMinVELOChi2ndof.Minimum(varPiminusOneVELOChi2ndof, varPiminusTwoVELOChi2ndof);
  _rdcr->CreateDoubleLeaf("varPionMinVELOChi2ndof", -999999.).Minimum(varPiplusMinVELOChi2ndof, varPiminusMinVELOChi2ndof);

  // T-stations track quality
  _rdcr->CreateDoubleCopyLeaf("varKminusTChi2ndof", _rdcr->GetInterimLeafByName("Dplus_Kminus_or_piminus_TRACK_TCHI2NDOF"));
  _rdcr->CreateDoubleCopyLeaf("varKplusTChi2ndof", _rdcr->GetInterimLeafByName("Dminus_Kplus_or_piplus_TRACK_TCHI2NDOF"));
  _rdcr->CreateDoubleLeaf("varKaonMinTChi2ndof", -999999.).Minimum(_rdcr->GetInterimLeafByName("Dplus_Kminus_or_piminus_TRACK_TCHI2NDOF"), _rdcr->GetInterimLeafByName("Dminus_Kplus_or_piplus_TRACK_TCHI2NDOF"));
  ReducerLeaf<Double_t>& varPiplusOneTChi2ndof = _rdcr->CreateDoubleCopyLeaf("varPiplusOneTChi2ndof", _rdcr->GetInterimLeafByName("Dplus_piplus_or_Kplus_One_TRACK_TCHI2NDOF"));
  ReducerLeaf<Double_t>& varPiplusTwoTChi2ndof = _rdcr->CreateDoubleCopyLeaf("varPiplusTwoTChi2ndof", _rdcr->GetInterimLeafByName("Dplus_piplus_or_Kplus_Two_TRACK_TCHI2NDOF"));
  ReducerLeaf<Double_t>& varPiplusMinTChi2ndof = _rdcr->CreateDoubleLeaf("varPiplusMinTChi2ndof", -999999.);
  varPiplusMinTChi2ndof.Minimum(varPiplusOneTChi2ndof, varPiplusTwoTChi2ndof);
  ReducerLeaf<Double_t>& varPiminusOneTChi2ndof = _rdcr->CreateDoubleCopyLeaf("varPiminusOneTChi2ndof", _rdcr->GetInterimLeafByName("Dminus_piminus_or_Kminus_One_TRACK_TCHI2NDOF"));
  ReducerLeaf<Double_t>& varPiminusTwoTChi2ndof = _rdcr->CreateDoubleCopyLeaf("varPiminusTwoTChi2ndof", _rdcr->GetInterimLeafByName("Dminus_piminus_or_Kminus_Two_TRACK_TCHI2NDOF"));
  ReducerLeaf<Double_t>& varPiminusMinTChi2ndof = _rdcr->CreateDoubleLeaf("varPiminusMinTChi2ndof", -999999.);
  varPiminusMinTChi2ndof.Minimum(varPiminusOneTChi2ndof, varPiminusTwoTChi2ndof);
  _rdcr->CreateDoubleLeaf("varPionMinTChi2ndof", -999999.).Minimum(varPiplusMinTChi2ndof, varPiminusMinTChi2ndof);

  // track match quality
  _rdcr->CreateDoubleCopyLeaf("varKminusMatchChi2ndof", _rdcr->GetInterimLeafByName("Dplus_Kminus_or_piminus_TRACK_MatchCHI2NDOF"));
  _rdcr->CreateDoubleCopyLeaf("varKplusMatchChi2ndof", _rdcr->GetInterimLeafByName("Dminus_Kplus_or_piplus_TRACK_MatchCHI2NDOF"));
  _rdcr->CreateDoubleLeaf("varKaonMinMatchChi2ndof", -999999.).Minimum(_rdcr->GetInterimLeafByName("Dplus_Kminus_or_piminus_TRACK_MatchCHI2NDOF"), _rdcr->GetInterimLeafByName("Dminus_Kplus_or_piplus_TRACK_MatchCHI2NDOF"));
  ReducerLeaf<Double_t>& varPiplusOneMatchChi2ndof = _rdcr->CreateDoubleCopyLeaf("varPiplusOneMatchChi2ndof", _rdcr->GetInterimLeafByName("Dplus_piplus_or_Kplus_One_TRACK_MatchCHI2NDOF"));
  ReducerLeaf<Double_t>& varPiplusTwoMatchChi2ndof = _rdcr->CreateDoubleCopyLeaf("varPiplusTwoMatchChi2ndof", _rdcr->GetInterimLeafByName("Dplus_piplus_or_Kplus_Two_TRACK_MatchCHI2NDOF"));
  ReducerLeaf<Double_t>& varPiplusMinMatchChi2ndof = _rdcr->CreateDoubleLeaf("varPiplusMinMatchChi2ndof", -999999.);
  varPiplusMinMatchChi2ndof.Minimum(varPiplusOneMatchChi2ndof, varPiplusTwoMatchChi2ndof);
  ReducerLeaf<Double_t>& varPiminusOneMatchChi2ndof = _rdcr->CreateDoubleCopyLeaf("varPiminusOneMatchChi2ndof", _rdcr->GetInterimLeafByName("Dminus_piminus_or_Kminus_One_TRACK_MatchCHI2NDOF"));
  ReducerLeaf<Double_t>& varPiminusTwoMatchChi2ndof = _rdcr->CreateDoubleCopyLeaf("varPiminusTwoMatchChi2ndof", _rdcr->GetInterimLeafByName("Dminus_piminus_or_Kminus_Two_TRACK_MatchCHI2NDOF"));
  ReducerLeaf<Double_t>& varPiminusMinMatchChi2ndof = _rdcr->CreateDoubleLeaf("varPiminusMinMatchChi2ndof", -999999.);
  varPiminusMinMatchChi2ndof.Minimum(varPiminusOneMatchChi2ndof, varPiminusTwoMatchChi2ndof);
  _rdcr->CreateDoubleLeaf("varPionMinMatchChi2ndof", -999999.).Minimum(varPiplusMinMatchChi2ndof, varPiminusMinMatchChi2ndof);
}
