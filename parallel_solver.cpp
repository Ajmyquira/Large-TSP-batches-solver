#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <thread>
#include <string>

using namespace std;
namespace fs = filesystem;

string int_to_string(int number, int digits)
{
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
    string filename = "LKH-2.0.9/cities" + index + ".tsp";
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
    string filename = "LKH-2.0.9/params" + index + ".par";
    fstream outfile;

    outfile.open(filename, std::ios_base::out);
    if (!outfile.is_open()) {
        cout << "failed to open " << filename << '\n';
    } else {
        outfile << "PROBLEM_FILE = cities" << index << ".tsp" << endl;
        outfile << "OUTPUT_TOUR_FILE = tsp_solution" << index << ".csv" << endl;
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

//void solver(string filename, string index, double* score){
void solver(string filename, string index){
    vector<string> x;
    vector<string> y;
    read_CSV(filename, &x, &y);
    write_TSP(x, y, index);
    write_parameters(index);
    string command = "cd .\\LKH-2.0.9 & .\\LKH params" + index + ".par & cls";
    const char* _command = command.c_str();
    system(_command);
    string file_name = "LKH-2.0.9/tsp_solution" + index + ".csv";
    //*score = score_tour(file_name);
    double score = score_tour(file_name);
    cout << "--------------------------------" << score << endl;
}

double parallel_solver(vector<string> paths){
    vector<thread> threadVect;
    double* scores = new double [paths.size()];
	//unsigned int threadSpread = N / numThreads;
	for (unsigned int i = 0; i < paths.size(); i++) {
        string index = int_to_string(i, 3);
		//threadVect.emplace_back(solver, paths[i], index, scores[i]);
        threadVect.emplace_back(solver, paths[i], index);
	}
	for (auto& t : threadVect) {
		t.join();
	}
    /*for(unsigned int i = 0; i < paths.size(); i++){
        cout << scores[i] << endl;
    }*/
    // Find the smallest score
    //
    delete[] scores;
    return 0;
}

int main(){
    string path = "dataset";
    vector<string> paths;
    // Collect the paths of all csv files within dataset
    for (const auto & entry : fs::directory_iterator(path)){
        //cout << entry.path() << endl;
        string path_string = entry.path().string();
        paths.push_back(path_string);
    }

    // Parallel part
    unsigned int number_threads = thread::hardware_concurrency();
    cout << "Number of threads: " << number_threads << endl;
    double final_score;
    //T0 = clock();
    final_score = parallel_solver(paths);
	//T1 = clock();

    //double time = (double(T1 - T0) / CLOCKS_PER_SEC);
	//cout << "Execution time: " << time << endl;

    return 0;
}