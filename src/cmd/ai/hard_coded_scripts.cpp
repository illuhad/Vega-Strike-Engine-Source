#include "script.h"
#include "cmd/unit_generic.h"
#include "hard_coded_scripts.h"
#include "flybywire.h"
#include "navigation.h"
#include "tactics.h"
#include "fire.h"
#include "order.h"
#include "python/python_class.h"
#include "vs_random.h"
using Orders::FireAt;

BOOST_PYTHON_BEGIN_CONVERSION_NAMESPACE

#ifdef USE_BOOST_128
extern PyObject *to_python (Unit *x);
#endif

BOOST_PYTHON_END_CONVERSION_NAMESPACE
bool useAfterburner () {
	static bool useafterburner = XMLSupport::parse_bool(vs_config->getVariable("AI","use_afterburner","true"));
	return useafterburner;
}
void AddOrd (Order *aisc, Unit * un, Order * ord) {
  ord->SetParent (un);
  aisc->EnqueueOrder (ord);
}
void ReplaceOrd (Order *aisc, Unit * un, Order * ord) {
  ord->SetParent (un);
  aisc->ReplaceOrder (ord);
}
static Order * lastOrder=NULL;
void FireAt::AddReplaceLastOrder (bool replace) {
	if (lastOrder) {
		if (replace) {
			ReplaceOrd (this,parent,lastOrder);
		}else {
			AddOrd (this,parent,lastOrder);
		}
		lastOrder=NULL;
	}
}
void FireAt::ExecuteLastScriptFor(float time) {
	if (lastOrder) {
		lastOrder = new ExecuteFor(lastOrder,time);
	}
}
void FireAt::FaceTarget (bool end) {
	lastOrder = new Orders::FaceTarget(end,4);
}
void FireAt::FaceTargetITTS (bool end) {
	lastOrder  = new Orders::FaceTargetITTS(end,4);
}
void FireAt::MatchLinearVelocity(bool terminate, Vector vec, bool afterburn, bool local) {
	afterburn =afterburn&&useAfterburner();
	lastOrder  = new Orders::MatchLinearVelocity(parent->ClampVelocity(vec,afterburn),local,afterburn,terminate);
}
void FireAt::MatchAngularVelocity(bool terminate, Vector vec, bool local) {
	lastOrder  = new Orders::MatchAngularVelocity(parent->ClampAngVel(vec),local,terminate);
}
void FireAt::ChangeHeading(QVector vec) {
	lastOrder  = new Orders::ChangeHeading (vec,3);
}
void FireAt::ChangeLocalDirection(Vector vec) {
	lastOrder  = new Orders::ChangeHeading (((parent->Position().Cast())+parent->ToWorldCoordinates(vec)).Cast(),3);
}
void FireAt::MoveTo(QVector vec, bool afterburn) {
	afterburn = afterburn&&useAfterburner();
	lastOrder  = new Orders::MoveTo(vec,afterburn,3);
}
void FireAt::MatchVelocity(bool terminate, Vector vec, Vector angvel, bool afterburn, bool local) {
	afterburn =afterburn&&useAfterburner();	
	lastOrder  = new Orders::MatchVelocity(parent->ClampVelocity(vec,afterburn),parent->ClampAngVel(angvel),local,afterburn,terminate);
}
void FireAt::Cloak(bool enable,float seconds) {
	lastOrder  = new CloakFor(enable,seconds);
}
void FireAt::FormUp(QVector pos) {
	lastOrder  = new Orders::FormUp(pos);
}
void FireAt::FaceDirection (float distToMatchFacing, bool finish) {
	lastOrder  = new Orders::FaceDirection (distToMatchFacing, finish, 3);
}
void FireAt::XMLScript (string script) {
	lastOrder  = new AIScript (script.c_str());
}
void FireAt::LastPythonScript () {
	lastOrder = PythonAI<Orders::FireAt>::LastPythonClass();
}

void AfterburnTurnTowards (Order * aisc, Unit * un) {
  Vector vec (0,0,10000);
  bool afterburn = useAfterburner();
  Order * ord = new Orders::MatchLinearVelocity(un->ClampVelocity(vec,afterburn),true,afterburn,false);
  AddOrd (aisc,un,ord);
  ord =       (new Orders::FaceTarget(false, 3));
  AddOrd (aisc,un,ord);    
}
void AfterburnTurnTowardsITTS (Order * aisc, Unit * un) {
  Vector vec (0,0,10000);

  bool afterburn = useAfterburner();
  Order * ord = new Orders::MatchLinearVelocity(un->ClampVelocity(vec,afterburn),true,afterburn,false);
  AddOrd (aisc,un,ord);
  ord =       (new Orders::FaceTargetITTS(false, 3));
  AddOrd (aisc,un,ord);    
}

void BarrelRoll (Order * aisc, Unit*un) {
	FlyByWire * broll = new FlyByWire;
	AddOrd(aisc,un,broll);
	broll->RollRight(rand()>RAND_MAX/2?1:-1);
	float per;
	if (rand()<RAND_MAX/2) {
		per=((float)rand())/RAND_MAX;
		if (per<.5) {
			per-=1;
		}
		broll->Up(per);
	}else {
		per=((float)rand())/RAND_MAX;
		if (per<.5) {
			per-=1;
		}		
		broll->Right(per);
	}
	bool afterburn = useAfterburner();
	broll->MatchSpeed(Vector(0,0,afterburn?un->GetComputerData().max_ab_speed():un->GetComputerData().max_speed()));
	broll->Afterburn(afterburn);
}

namespace Orders{
class LoopAround: public Orders::FaceTargetITTS{
	Orders::MoveToParent m;
	float qq;
	float pp;
	Vector rr;// place to go for @ end
	bool aggressive;
	bool afterburn;
	bool force_afterburn;
public:
	LoopAround(bool aggressive, bool afterburn, bool force_afterburn, int seed):FaceTargetITTS(false,3),m(false,2,false) {
		VSRandom vsr(seed);
		this->afterburn=afterburn;
		this->force_afterburn=force_afterburn;
		this->aggressive=aggressive;
		static float loopdis=XMLSupport::parse_float (vs_config->getVariable("AI","loop_around_distance","1"));
		qq=pp=0;
		static float loopdisd=XMLSupport::parse_float (vs_config->getVariable("AI","loop_around_destination_distance","10.0"));
		static float loopdisv=XMLSupport::parse_float (vs_config->getVariable("AI","loop_around_destination_vertical","4.0"));
		static float loopdisl=XMLSupport::parse_float (vs_config->getVariable("AI","loop_around_destination_lateral","4.0"));
		rr.Set(loopdisl*vsr.uniformInc(-1,1),loopdisv*vsr.uniformInc(-1,1),1.0+loopdisd*vsr.uniformInc(0,1));
		if (vsr.rand()<VS_RAND_MAX/2) {
			qq = vsr.uniformInc(-1,1);
			rr.j=qq;
			if (qq>0)
				qq+=loopdis;
			if (qq<0)
				qq-=loopdis;
		}else {
			pp = vsr.uniformInc(-1,1);
			rr.i=pp;
			if (pp>0)
				pp+=loopdis;
			if (pp<0)
				pp-=loopdis;
		}
	}
	void Execute(){
		Unit * targ = parent->Target();
		if (targ) {
			Vector relloc = parent->Position()-targ->Position();
			Vector r =targ->cumulative_transformation_matrix.getR();
			bool afterburn = useAfterburner()&&this->afterburn;
                        bool ab_needed=force_afterburn||targ->GetVelocity().MagnitudeSquared()>parent->GetComputerData().max_speed();
			if (r.Dot(relloc) <0) {
				FaceTargetITTS::Execute();
				m.SetAfterburn (afterburn&&ab_needed);
				m.Execute(parent,targ->Position()-r.Scale(rr.k*parent->rSize()+targ->rSize())+targ->cumulative_transformation_matrix.getP()*(rr.i*parent->rSize())+targ->cumulative_transformation_matrix.getQ()*(rr.j*parent->rSize()));
			}else {
				done=false;
				if (afterburn)
					m.SetAfterburn (ab_needed);
				else
					m.SetAfterburn(0);
				Vector scala=targ->cumulative_transformation_matrix.getQ().Scale(qq*(parent->rSize()+targ->rSize()))+targ->cumulative_transformation_matrix.getP().Scale(pp*(parent->rSize()+targ->rSize()));								
				QVector dest =targ->Position()+scala;
				if (aggressive){
					FaceTargetITTS::Execute();
				}else{
					SetDest(dest);					
					ChangeHeading::Execute();
				}
				m.Execute(parent,dest+scala);
			}
		}
	}
};
}
void LoopAround(Order* aisc, Unit * un) {
	Order* broll = new Orders::LoopAround(false,true,false,(int)un);
	AddOrd(aisc,un,broll);
	
}
void AggressiveLoopAround(Order* aisc, Unit * un) {
	Order* broll = new Orders::LoopAround(true,true,false,(int)un);
	AddOrd(aisc,un,broll);
	
}
void RollLeft(Order *aisc, Unit* un) {
  if (un->aistate)
    AddOrd(un->aistate,un,new ExecuteFor(new Orders::MatchRoll(un->GetComputerData().max_roll,false),1.0f));
}
void RollRight(Order *aisc, Unit* un) {
  if (un->aistate)
    AddOrd(un->aistate,un,new ExecuteFor(new Orders::MatchRoll(-un->GetComputerData().max_roll,false),1.0f));
}
void LoopAroundFast(Order* aisc, Unit * un) {
	Order* broll = new Orders::LoopAround(false,true,true,(int)un);
	AddOrd(aisc,un,broll);
	
}
void AggressiveLoopAroundFast(Order* aisc, Unit * un) {
	Order* broll = new Orders::LoopAround(true,true,true,(int)un);
	AddOrd(aisc,un,broll);
	
}
void LoopAroundSlow(Order* aisc, Unit * un) {
	Order* broll = new Orders::LoopAround(false,false,false,(int)un);
	AddOrd(aisc,un,broll);
	
}
void AggressiveLoopAroundSlow(Order* aisc, Unit * un) {
	Order* broll = new Orders::LoopAround(true,false,false,(int)un);
	AddOrd(aisc,un,broll);
	
}
void Evade(Order * aisc, Unit * un) {
  QVector v(un->Position());
  QVector u(v);
  Unit * targ =un->Target();
  if (targ) {
    u=targ->Position();
  }
  Order *ord = new Orders::ChangeHeading ((200*(v-u)) + v,3);
  AddOrd (aisc,un,ord);
  bool afterburn = useAfterburner();
  ord = new Orders::MatchLinearVelocity(un->ClampVelocity(Vector (-10000,0,10000),afterburn),false,afterburn,true);
  AddOrd (aisc,un,ord);
  ord = new Orders::FaceTargetITTS(false,3);
  AddOrd (aisc,un,ord);
  ord = new Orders::MatchLinearVelocity(un->ClampVelocity(Vector (10000,0,10000),afterburn),false, afterburn,true);  
  AddOrd (aisc,un,ord);
}
void MoveTo(Order * aisc, Unit * un) {
  QVector Targ (un->Position());
  Unit * untarg = un->Target();
  if (untarg) {
    Targ = untarg->Position();
  }
  Order * ord = new Orders::MoveTo(Targ,false,3);
  AddOrd (aisc,un,ord);
}

void KickstopBase(Order * aisc, Unit * un, bool match) {
  Vector vec (0,0,0);
  if (match&&un->Target())
    vec=un->Target()->GetVelocity();
  Order * ord = new Orders::MatchLinearVelocity(un->ClampVelocity(vec,false),true,false,true);
  AddOrd (aisc,un,ord);
  ord =       (new Orders::FaceTargetITTS(false, 3));
  AddOrd (aisc,un,ord);  
}
void Kickstop(Order * aisc, Unit * un) {
  KickstopBase(aisc,un,false);
}
void MatchVelocity(Order * aisc, Unit * un) {
  KickstopBase(aisc,un,false);
}
static Vector VectorThrustHelper(Order * aisc, Unit * un, bool ab=false) {
  Vector vec (0,0,0);
  Vector retval(0,0,0);
  if (un->Target()) {
    Vector tpos=un->Target()->Position().Cast();
    Vector relpos=tpos-un->Position().Cast();
    CrossProduct(relpos,Vector(1,0,0),vec);
    retval+=tpos;
  }
  Order * ord = new Orders::MatchLinearVelocity(un->ClampVelocity(vec,ab),true,ab,true);
  AddOrd (aisc,un,ord);
  return retval;
}
void VeerAway(Order * aisc, Unit * un) {
  VectorThrustHelper (aisc,un);
  Order *ord =       (new Orders::FaceTarget(false, 3));
  AddOrd (aisc,un,ord);  
}
void VeerAwayITTS(Order * aisc, Unit * un) {
  VectorThrustHelper (aisc,un);
  Order *ord =       (new Orders::FaceTargetITTS(false, 3));
  AddOrd (aisc,un,ord);  
}
void VeerAndTurnAway(Order * aisc, Unit * un) {
  Vector retval=VectorThrustHelper (aisc,un);
  Order *ord = new Orders::ChangeHeading(retval,3,1);
  AddOrd (aisc,un,ord);  
}
void AfterburnVeerAndTurnAway(Order * aisc, Unit * un) {
  Vector retval=VectorThrustHelper (aisc,un,true);
  Order *ord = new Orders::ChangeHeading(retval,3,1);
  AddOrd (aisc,un,ord);  
}
static void SetupVAndTargetV (QVector & targetv, QVector &targetpos, Unit* un) {
  Unit *targ;
  if ((targ = un->Target())) {
    targetv = targ->GetVelocity().Cast();
    targetpos = targ->Position();
  }  
}

void SheltonSlide(Order * aisc, Unit * un) {
  QVector def (un->Position()+QVector(1,0,0));
  QVector targetv(def);
  QVector targetpos(def);
  SetupVAndTargetV(targetpos,targetv,un);
  QVector difference = targetpos-un->Position();
  QVector perp = targetv.Cross (-difference);
  perp.Normalize();
  perp=perp*(targetv.Dot(difference*-1./(difference.Magnitude())));
  perp =(perp+difference)*10000.;

  bool afterburn=useAfterburner();
  Order * ord = new Orders::MatchLinearVelocity(un->ClampVelocity(perp.Cast(),afterburn),false,afterburn,true);  
  AddOrd (aisc,un,ord);
  ord =       (new Orders::FaceTargetITTS(false, 3));
  AddOrd (aisc,un,ord);
}

void AfterburnerSlide(Order * aisc, Unit * un) {
  QVector def = un->Position()+QVector(1,0,0);
  QVector targetv (def);
  QVector targetpos(def);
  SetupVAndTargetV(targetpos,targetv,un);

  QVector difference = targetpos-un->Position();
  QVector perp = targetv.Cross (-difference);
  perp.Normalize();
  perp=perp*(targetv.Dot(difference*-1./(difference.Magnitude())));
  perp =(perp+difference)*1000;
  bool afterburn=useAfterburner();
  Order * ord = new Orders::MatchLinearVelocity(un->ClampVelocity(perp.Cast(),afterburn),false,afterburn,true);  
  AddOrd (aisc,un,ord);
  ord = new ExecuteFor (new Orders::ChangeHeading (perp+un->Position(),3),1.5);
  AddOrd (aisc,un,ord);
  ord =       (new Orders::FaceTargetITTS(false, 3));
  AddOrd (aisc,un,ord);
}
void SkilledABSlide (Order * aisc, Unit * un) {
  QVector def = un->Position()+QVector(1,0,0);
  QVector targetv (def);
  QVector targetpos (def);
  SetupVAndTargetV(targetpos,targetv,un);

  QVector difference = targetpos-un->Position();
  QVector ndifference = difference;ndifference.Normalize();
  QVector Perp;
  ScaledCrossProduct (ndifference,targetv,Perp);
  Perp = Perp+.5*ndifference;
  Perp = Perp *10000;

  bool afterburn=useAfterburner();

  Order * ord = new Orders::MatchLinearVelocity(un->ClampVelocity(Perp.Cast(),afterburn),false,afterburn,true);  
  AddOrd (aisc,un,ord);
  ord = new ExecuteFor (new Orders::ChangeHeading (Perp+un->Position(),3),.5);
  AddOrd (aisc,un,ord);
  ord =       (new Orders::FaceTargetITTS(false, 3));
  AddOrd (aisc,un,ord);
  
}
void Stop (Order * aisc, Unit * un) {
  Vector vec (0,0,0000);
  Order * ord = new Orders::MatchLinearVelocity(un->ClampVelocity(vec,false),true,false,false);
  AddOrd (aisc,un,ord);//<!-- should we fini? -->
}
void AfterburnTurnAway(Order * aisc, Unit * un) {
  QVector v(un->Position());
  QVector u(v);
  Unit * targ =un->Target();
  if (targ) {
    u=targ->Position();
  }
  bool afterburn = useAfterburner();
  Order * ord = new Orders::MatchLinearVelocity(un->ClampVelocity(200*(v-u).Cast(),afterburn),false,afterburn,false);
  AddOrd (aisc,un,ord);
  ord = new Orders::ChangeHeading ((200*(v-u)) + v,3);
  AddOrd (aisc,un,ord);
}
void TurnAway(Order * aisc, Unit * un) {
  QVector v(un->Position());
  QVector u(v);
  Unit * targ =un->Target();
  if (targ) {
    u=targ->Position();
  }
  bool afterburn = false;
  Order * ord = new Orders::MatchLinearVelocity(un->ClampVelocity(200*(v-u).Cast(),afterburn),false,afterburn,false);
  AddOrd (aisc,un,ord);
  ord = new Orders::ChangeHeading ((200*(v-u)) + v,3);
  AddOrd (aisc,un,ord);
}
void TurnTowards(Order * aisc, Unit * un) {
  Vector vec (0,0,10000);
  Order * ord = new Orders::MatchLinearVelocity(un->ClampVelocity(vec,false),true,false,false);
  AddOrd (aisc,un,ord);

  ord = new Orders::FaceTarget(0, 3);
    AddOrd (aisc,un,ord);
}
void FlyStraight(Order * aisc, Unit * un) {
  Vector vec (0,0,10000);
  Order * ord = new Orders::MatchVelocity(un->ClampVelocity(vec,false),Vector(0,0,0),true,false,false);
  AddOrd (aisc,un,ord);
  ord = new Orders::MatchAngularVelocity(Vector(0,0,0),1,false);
  AddOrd (aisc,un,ord);
}
void FlyStraightAfterburner(Order * aisc, Unit * un) {
  Vector vec (0,0,10000);
  bool afterburn=useAfterburner();
  Order * ord = new Orders::MatchVelocity(un->ClampVelocity(vec,afterburn),Vector(0,0,0),true,afterburn,false);
  AddOrd (aisc,un,ord);
  ord = new Orders::MatchAngularVelocity(Vector(0,0,0),1,false);
  AddOrd (aisc,un,ord);
}
void CloakForScript(Order * aisc, Unit * un) {
  Vector vec (0,0,10000);
  Order * ord = new Orders::MatchLinearVelocity(un->ClampVelocity(vec,false),true,false,false);
  AddOrd (aisc,un,ord);
  ord =       (new Orders::FaceTargetITTS(0, 3));
  AddOrd (aisc,un,ord);
  ord=new ExecuteFor(new CloakFor(1,8),32);
  AddOrd(aisc,un,ord);
}
void TurnTowardsITTS(Order * aisc, Unit * un) {
  Vector vec (0,0,10000);
  Order * ord = new Orders::MatchLinearVelocity(un->ClampVelocity(vec,false),true,false,false);
  AddOrd (aisc,un,ord);
  ord =       (new Orders::FaceTargetITTS(0, 3));
  AddOrd (aisc,un,ord);    

}
void DropCargo(Order*aisc, Unit * un) {
      if (un->numCargo() > 0)
      {
              int dropcount=(un->numCargo()/10)+1;

              {for(int i=0; i<dropcount; i++)
              {
                      un->EjectCargo(0);  
              }}
              Stop(aisc,un);
      }
      else
      {
              TurnAway(aisc, un);
      }
}

