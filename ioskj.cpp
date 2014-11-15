#define DEBUG 0

#include "imports.hpp"
#include "dimensions.hpp"
#include "model.hpp"
#include "parameters.hpp" 
#include "data.hpp"
#include "procedures.hpp"
#include "performance.hpp"
#include "tracker.hpp"

using namespace IOSKJ;

/**
 * Run the model with a parameters set read from "parameters/input"
 *
 * @param samples_file A filesystem path to a TSV file of parameter samples
 * @parameters samples_row Row index of samples to select
 */
void run(const std::string& samples_file="", int samples_row=0){
	// Create output directories
	boost::filesystem::create_directories("model/output");
	// Read in parameters
	Parameters parameters;
	parameters.read();
	// If samples is specified, read them in and select the desired row
	if(samples_file!=""){
		// Read in samples
		Frame samples;
		samples.read(samples_file);
		//... select desired row
		Frame row = samples.slice(samples_row);
		//... overwrite parameters available, ignoring catches
		parameters.read(row,{"catches"});
	}
	// Read in data
	Data data;
	data.read();
	// Do tracking
	Tracker tracker("model/output/track.tsv");
	// Instantiate a model
	Model model;
	// For each time step...
	for(uint time=0;time<=time_max;time++){
		std::cout<<time<<"\t"<<year(time)<<"\t"<<quarter(time)<<std::endl;
		//... set model parameters
		parameters.set(time,model);
		//... update the model
		model.update(time);
		//... get model variables corresponding to data
		data.get(time,model);
		//... get model variables of interest for tracking
		tracker.get(0,0,time,model);
	}
	// Write out
	model.write();
	parameters.write();
	data.write();
}

/**
 * Perform yield curve and MSY calculations with a parameters set read from "parameters/input"
 */
void yield(void){
	boost::filesystem::create_directories("yield/output");

	Model model;
	// Read in and set parameters
	Parameters parameters;
	parameters.read();
	parameters.set(0,model);
	// Generate and output yield curve
	Frame yc = model.yield_curve();
	yc.write("yield/output/curve.tsv");
	// Find MSY and output
	model.msy_find();
	std::ofstream file("yield/output/model.tsv");
	file<<"e_msy\tf_msy\tmsy\tbiomass_spawning_msy\tmsy_trials\n";
	file<<model.e_msy<<"\t"<<model.f_msy<<"\t"<<model.msy<<"\t"<<model.biomass_spawners_msy<<"\t"<<model.msy_trials<<"\n";
}

/**
 * Generate samples from the parameter prior distributions 
 */
void priors(int replicates=1000){
	boost::filesystem::create_directories("parameters/output");

	Parameters parameters;
	parameters.read();
	Frame samples;
	for(int replicate=0;replicate<replicates;replicate++){
		parameters.randomise();
		samples.append(parameters.values());
	}
	samples.write("parameters/output/priors.tsv");
}

/**
 * Check feasibility constraints for a model
 */
typedef int (Check)(const Model& model, const Data& data, uint time, uint year, uint quarter);
void check(Check check, int trial, Parameters& parameters, Data& data, Tracker& tracker, Frame& accepted, Frame& rejected){
	Model model;
	uint time = 0;
	uint time_end = IOSKJ::time(2013,3);
	for(;time<=time_end;time++){
		// Do the time step
		//... set parameters
		parameters.set(time,model);
		//... update the model
		model.update(time);
		//... get data
		data.get(time,model);
		//... do tracking
		if(trial<100) tracker.get(trial,-1,time,model);
		//... check model
		uint year = IOSKJ::year(time);
		uint quarter = IOSKJ::quarter(time);
		int criterion = check(model,data,time,year,quarter);
		if(criterion!=0){
			Frame values = parameters.values();
			values.add("time",time);
			values.add("year",year);
			values.add("quarter",quarter);
			values.add("criterion",criterion);
			rejected.append(values);
			break;
		}
		if(time==time_end){
			accepted.append(parameters.values());
		}
	}
	if(trial>0 and trial%10==0) std::cout<<trial<<" "<<accepted.rows()/float(trial)<<std::endl;
}


/**
 * Check feasibility constraints
 *
 * Used in the `condition _feasible` method
 */
int check_feasible(const Model& model, const Data& data, uint time, uint year, uint quarter){
	// Stock status ...
	auto status = model.biomass_spawning_overall(quarter)/model.biomass_spawning_unfished(quarter);
	// ... must always be >10% B0
	if(status<0.1) return 1;
	// ... since 2008 must be less than 100% B0
	if(year>2008 and status>1) return 2;

	// M PL CPUE ...
	static double m_pl_cpue_base;
	if(year==2004 and quarter==2) m_pl_cpue_base = data.m_pl_cpue(year,quarter);
	// ... must have increased from 2004 to 2006
	if(year==2006 and quarter==2 and data.m_pl_cpue(year,quarter)/m_pl_cpue_base<1) return 3;
	// ... must have decreased from 2004 to 2011
	if(year==2011 and quarter==2 and data.m_pl_cpue(year,quarter)/m_pl_cpue_base>1) return 4;

	// Z-estimates
	if(year>=2006 and year<=2009){
		if(data.z_ests(year,quarter,1)<0.2 or data.z_ests(year,quarter,1)>0.7) return 5;
	}

	return 0;
}

/**
 * Condition based on feasibility constraints
 */
void condition_feasible(int trials=100){
	// Create output directory
	boost::filesystem::create_directories("feasible/output");
	// Read in parameter priors and default values
	Parameters parameters;
	parameters.read();
	// Read in data
	Data data;
	data.read();
	// Frames for accepted and rejected parameter samples
	Frame accepted;
	Frame rejected;
	// Do tracking (for a subset of trials)
	Tracker tracker("feasible/output/track.tsv");
	// Do a number of trial parameter samples
	for(int trial=0;trial<trials;trial++){
		// Randomly sample parameters from priors
		parameters.randomise();
		// Check feasibility of parameters
		check(check_feasible,trial,parameters,data,tracker,accepted,rejected);
	}
	// Write out
	accepted.write("feasible/output/accepted.tsv");
	rejected.write("feasible/output/rejected.tsv");
}

/**
 * Check SS3 model run
 *
 * Used in the `condition_ss3` method
 */
int check_ss3(const Model& model, const Data& data, uint time, uint year, uint quarter){

	// Stock status ...
	auto status = model.biomass_spawning_overall(quarter)/model.biomass_spawning_unfished(quarter);
	// ... must always be >10% B0
	if(status<0.1) return 1;
	// ... since 2008 must be less than 100% B0
	if(year>2008 and status>1) return 2;

	return 0;
}

/**
 * Generate samples based on the SS3 assessment grid with priors used for those
 * paramters not available from there.
 */
void condition_ss3(int replicates=1000){
	// Create output directory
	boost::filesystem::create_directories("ss3/output");
	// Read in parameter priors and default values
	Parameters parameters;
	parameters.read();
	// Read in data
	Data data;
	data.read();
	// Read in parameter values from SS3 grid
	Frame grid;
	grid.read("ss3/pars.tsv");
	// Frames for accepted and rejected parameter samples
	Frame accepted;
	Frame rejected;
	// Do tracking (for a subset of trials)
	Tracker tracker("ss3/output/track.tsv");
	// For each replicate...
	for(int replicate=0;replicate<replicates;replicate++){
		//... randomise parameter values from priors
		parameters.randomise();
		//... randomly choose a grid cell
		Frame cell = grid.slice(
			Uniform(0,grid.rows()).random()
		);
		//... overwrite parameters avialable from grid
		parameters.read(cell,{"catches"});
		//... check feasibility of parameters
		check(check_ss3,replicate,parameters,data,tracker,accepted,rejected);
	}
	accepted.write("ss3/output/accepted.tsv");
	rejected.write("ss3/output/rejected.tsv");
}

/**
 * Evaluate management procedures
 */
void evaluate(const std::string& samples_file, int replicates=1000){
	boost::filesystem::create_directories("evaluate/output");
	// Setup parameters and data
	Parameters parameters;
	parameters.read();
	Data data;
	data.read();
	// Read in samples from conditioning and 
	// create a frame for storing selected samples
	Frame samples_all;
	samples_all.read(samples_file);
	samples_all.write("evaluate/output/samples_all.tsv");
	Frame samples;
	// Setup procedures
	Procedures procedures;
	procedures.populate();
	// Setup performance statistics
	Array<Performance> performances;
	// Do tracking (for a subset of replicates)
	Tracker tracker("evaluate/output/track.tsv");
	// For each replicate...
	for(int replicate=0;replicate<replicates;replicate++){
		std::cout<<replicate<<std::endl;
		// Randomly select a parameter sample
		Frame sample = samples_all.slice(
			Uniform(0,samples_all.rows()).random()
		);
		samples.append(sample);
		// Read parameters from sample 
		// (to save time don't attempt to read catches array)
		parameters.read(sample,{"catches"});
		// Create a model representing current state by iterating
		// from time 0 to now...
		Model current;
		for(uint time=0;time<=time_now;time++){
			//... set parameters
			parameters.set(time,current); 
			//... update the model
			current.update(time);
			//... track the model (for speed, only track some replicates)
			if(replicate<100) tracker.get(replicate,-1,time,current);
		}
		// Determine MSY related reference points
		current.msy_find();
		// Generate a random seed to be used to ensure future
		// variability is same for all procedures
		uint seed = Uniform(0,1e10).random();
		// For each candidate procedure...
		for(uint procedure=0;procedure<procedures.size();procedure++){
			// Create a model with current state to use to 
			// simulate procedure
			Model future = current;
			// Set up performance statistics
			Performance performance(replicate,procedure);
			// Reset random seed
			Generator.seed(seed);
			// Reset the procedure
			procedures.reset(procedure);
			// Iterate over years...
			for(uint time=time_now+1;time<=time_max;time++){
				//... set parameters
				parameters.set(time,current);
				//... update the model
				future.update(time);
				//... track the model (for speed, only some replicates)
				if(replicate<100) tracker.get(replicate,procedure,time,future);
				//... operate the procedure
				procedures.operate(procedure,time,future);
				//... record performance
				performance.record(time,future);
			}
			// Save performance
			performances.append(performance);
		}
	}
	procedures.write("evaluate/output/procedures.tsv");
	samples.write("evaluate/output/samples.tsv");
	performances.write("evaluate/output/performances.tsv");
}

int main(int argc, char** argv){ 
	try {
        if(argc==1) throw std::runtime_error("No task given");
        std::string task = argv[1];
        uint num = (argc>2)?boost::lexical_cast<uint>(argv[2]):0;
        std::cout<<"-------------"<<task<<"-------------\n";
        if(task=="run") run();
        else if(task=="run_ss3") run("ss3/pars.tsv",num);
        else if(task=="yield") yield();
        else if(task=="priors") priors();
        else if(task=="condition_feasible") condition_feasible(num);
        else if(task=="condition_ss3") condition_ss3(num);
        else if(task=="evaluate_feasible") evaluate("feasible/output/accepted.tsv",num);
        else if(task=="evaluate_ss3") evaluate("ss3/output/accepted.tsv",num);
        else throw std::runtime_error("Unrecognised task");
        std::cout<<"-------------------------------\n";
	} catch(std::exception& error){
        std::cout<<"************Error************\n"
                <<error.what()<<"\n"
                <<"******************************\n";
	} catch(...){
        std::cout<<"************Unknown error************\n";
	}
	return 0;
}
