#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <thread>
#include <string>
#include <mutex>
#include <chrono>

using namespace std;
namespace fs = filesystem;

mutex m;

// Convert an integer to a string with a specific number of digits
string int_to_string(int number, int digits){
    string ans = "";
    while (number)
    {
        ans.push_back('0' + (number % 10));
        number /= 10;
    }
    reverse(ans.begin(), ans.end());
    return string(digits - ans.size(), '0') + ans;
}

void read_CSV(string argv, vector<string>* vectorX, vector<string>* vectorY){
    ifstream file(argv);
    string line;
    char delimitator = ',';
    getline(file, line);

    while(getline(file, line)){
        stringstream stream(line);
        string index, x, y;
        getline(stream, index, delimitator);
        getline(stream, x, delimitator);
        getline(stream, y, delimitator);

        //cout << "Index:" << index << endl;
        //cout << "x: " << x << endl;
        //cout << "y: " << y << endl;

        (*vectorX).push_back(x);
        (*vectorY).push_back(y);
    }
}

void write_TSP(vector<string> vectorX, vector<string> vectorY, string index){
    string filename = "LKH-2.0.9/cities_tsp/cities" + index + ".tsp";
    fstream outfile;

    outfile.open(filename, std::ios_base::out);
    if (!outfile.is_open()) {
        cout << "failed to open " << filename << '\n';
    } else {
        outfile << "NAME : traveling-santa-2018-prime-paths" << endl;
        outfile << "COMMENT : traveling-santa-2018-prime-paths" << endl;
        outfile << "TYPE : TSP" << endl;
        outfile << "DIMENSION : " << vectorX.size() << endl;
        outfile << "EDGE_WEIGHT_TYPE : EUC_2D" << endl;
        outfile << "NODE_COORD_SECTION" << endl;
        for(unsigned int i = 0; i < vectorX.size(); i++){
            outfile << i+1 << " " << vectorX[i] << " " << vectorY[i] << endl;
        }
        outfile << "EOF" << endl;
        //cout << "Done Writing!" << endl;
    }
}

void write_parameters(string index){
    string filename = "LKH-2.0.9/params_par/params" + index + ".par";
    fstream outfile;

    outfile.open(filename, std::ios_base::out);
    if (!outfile.is_open()) {
        cout << "failed to open " << filename << '\n';
    } else {
        outfile << "PROBLEM_FILE = cities_tsp/cities" << index << ".tsp" << endl;
        outfile << "OUTPUT_TOUR_FILE = solution_csv/tsp_solution" << index << ".csv" << endl;
        outfile << "SEED = 2018" << endl;
        outfile << "CANDIDATE_SET_TYPE = POPMUSIC" << endl;
        outfile << "INITIAL_PERIOD = 10000" << endl;
        outfile << "MAX_TRIALS = 1000" << endl;
        //cout << "Done Writing!" << endl;
    }
}

double score_tour(string filename){
    double fiscore;
    ifstream file(filename);
    string line;
    string delimitator = "Length = ";
    getline(file, line);
    getline(file, line);
    string score_value = line.substr(19);
    //cout << score << endl;
    return stod(score_value);
}

void solver(vector<string>* paths, unsigned int i, unsigned int thread_spread, double* best_score){
    double local_best_score = INT_MAX;

    for (unsigned int j = i * thread_spread; j < (i + 1) * thread_spread; j++){
        vector<string> x; // Store the x values of the coordenates
        vector<string> y; // Store the y values of the coordenates
        
        string index = int_to_string(j, 4);

        read_CSV((*paths)[j], &x, &y);
        write_TSP(x, y, index);
        write_parameters(index);

        // Execute the LKH solver
        string command = "cd .\\LKH-2.0.9 & .\\LKH params_par/params" + index + ".par";
        const char* _command = command.c_str();
        system(_command);

        string file_name = "LKH-2.0.9/solution_csv/tsp_solution" + index + ".csv";
        // Read the store of the solution
        double score = score_tour(file_name);
        //cout << "\nScore [" << index << "]: " << score << "----------------here!" << endl;
        
        
        // Determine if the score calculated is the best score
        if(score < local_best_score) local_best_score = score;
    }
    // Critical section
    m.lock();
    if (local_best_score < *best_score) *best_score = local_best_score;
    m.unlock();
}

double parallel_solver(vector<string> paths, unsigned int num_threads){
    vector<thread> threadVect;
    unsigned int thread_spread = paths.size() / num_threads;
    double best_score = INT_MAX;

	for (unsigned int i = 0; i < num_threads; i++) {
        //string index = int_to_string(i, 4);
        //vector<string> sub_paths = paths.substr(i * thread_spread, thread_spread);
        threadVect.emplace_back(solver, &paths, i, thread_spread, &best_score);
	}

	for (auto& t : threadVect) {
		t.join();
	}
    //cout << best_score << endl;
    return best_score;
}

int main(){
    string path = "dataset"; // Folder containing the cvs. files
    vector<string> paths; // Store all cvs. files paths
    
    // Collect the paths of all csv files within dataset
    for (const auto & entry : fs::directory_iterator(path)){
        //cout << entry.path() << endl;
        string path_string = entry.path().string();
        paths.push_back(path_string);
    }

    // Parallel part
    unsigned int number_threads = thread::hardware_concurrency();
    cout << "Number of threads: " << number_threads << endl;
    double best_score;
    
    chrono::time_point<std::chrono::high_resolution_clock> start, end;
    start = chrono::high_resolution_clock::now();
    best_score = parallel_solver(paths, number_threads);
	end = chrono::high_resolution_clock::now();
    int64_t duration = chrono::duration_cast<chrono::seconds>(end - start).count();
    cout << endl << setw(10) << "Duration: " + to_string(duration) + " s\n";

    cout << "Best score: " << best_score << endl;
    
    // Write the final score
    string filename = "final_score.txt";
    fstream outfile;
    outfile.open(filename, std::ios_base::out);
    if (!outfile.is_open()) {
        cout << "failed to open " << filename << '\n';
    } else {
        outfile << best_score;
        //cout << "Done Writing!" << endl;
    }

    return 0;
}