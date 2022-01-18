#include "application.hpp"
#include "instance.hpp"
#include "lower_bounds.hpp"
#include "../algos/algosTS.hpp"

#include <iostream>
#include <fstream>
#include <chrono>

using namespace std;
using namespace std::chrono;


std::string run_for_instance(const InstanceTS & instance,
                             const vector<string> & list_algos,
                             const vector<string> & list_spread)
{
    int LB_cpu, LB_mem;
    TS_LB(instance, LB_cpu, LB_mem);

    int LB = std::max(LB_cpu, LB_mem);
    int hint_bin = LB + 500;
    int best_sol = instance.getTotalReplicas();
    string best_algo;

    string row(to_string(LB));
    string row_res;
    string row_time;

    int sol;
    for (const string & algo_name : list_algos)
    {
        AlgoFitTS * algo = createAlgoTS(algo_name, instance);
        if (algo != nullptr)
        {
            auto start = high_resolution_clock::now();
            sol = algo->solveInstance(hint_bin);
            auto stop = high_resolution_clock::now();
            auto duration = duration_cast<seconds>(stop - start);

            if (sol < best_sol)
            {
                best_sol = sol;
                best_algo = algo_name;
            }

            row_res.append("\t" + to_string(sol));
            row_time.append("\t" + to_string((float)duration.count()));

            delete algo;
        }
        else
        {
            cout << "Unknown algo name: " << algo_name << endl;
        }
    }

    row.append("\t"+to_string(best_sol));

    // Always take solution of FirstFit as upper bound input
    AlgoFitTS* algoFF = createAlgoTS("FF", instance);
    int UB = algoFF->solveInstance(hint_bin);
    delete algoFF;
    for (const string & algo_name : list_spread)
    {
        AlgoTSSpreadWFDAvg * algo = createSpreadAlgo(algo_name, instance);
        if (algo != nullptr)
        {
            auto start = high_resolution_clock::now();
            sol = algo->solveInstanceSpread(LB, UB);
            auto stop = high_resolution_clock::now();
            auto duration = duration_cast<seconds>(stop - start);

            if ((sol < best_sol) and (sol != -1))
            {
                best_sol = sol;
                best_algo = algo_name;
            }

            row_res.append("\t" + to_string(sol));
            row_time.append("\t" + to_string((float)duration.count()));

            delete algo;
        }
        else
        {
            cout << "Unknown algo name: " << algo_name << endl;
        }
    }

    row.append("\t" + to_string(best_sol) + "\t" + best_algo);
    return row + row_res + row_time;
}


int run_list_algos(string input_path, string& outfile,
                   vector<string>& list_algos, vector<string>& list_spread,
                   int bin_cpu_capacity, int bin_mem_capacity,
                   int density, string graph)
{
    ofstream f(outfile, ios_base::trunc);
    if (!f.is_open())
    {
        cout << "Cannot write file " << outfile << endl;
        return -1;
    }
    cout << "Writing output to file " << outfile << endl;

    // Header line
    string header("instance_name\tLB\tbest_sol\tbest_spread\tbest_algo");
    string time_header;

    for (std::string algo_name : list_algos)
    {
        header.append("\t" + algo_name);
        time_header.append("\t" + algo_name + "_time");
    }
    for (std::string algo_name : list_spread)
    {
        header.append("\t" + algo_name);
        time_header.append("\t" + algo_name + "_time");
    }
    f << header << time_header << "\n";

    vector<int> densities;// = { 1, 5, 10 };
    densities.push_back(density);
    vector<string> graph_classes;// = { "arbitrary", "normal", "threshold" };
    graph_classes.push_back(graph);

    size_t size_series = 98;

    for (int d : densities)
    {
        for (string& graph_class : graph_classes)
        {
            cout << "Starting density " << to_string(d) << " graph class: " << graph_class << endl;
            for (int n = 0; n < 10; ++n)
            {
                cout << to_string(n) << " ";
                string instance_name(graph_class + "_d" + to_string(d) + "_" + to_string(n));
                string infile(input_path + instance_name + ".csv");
                const InstanceTS instance(instance_name, bin_cpu_capacity, bin_mem_capacity, infile, size_series);

                string row_str = run_for_instance(instance, list_algos, list_spread);
                f << instance_name << "\t" << row_str << "\n";
                f.flush();
            }
            cout << endl;
        }
    }

    f.close();
    return 0;
}


int main(int argc, char** argv)
{
    string input_path = "/nobackup/scscm/TClab_data/densityTS/";
    string output_path = "/nobackup/scscm/new_outputs/";

    int bin_cpu_capacity;
    int bin_mem_capacity;
    int density;
    string graph;
    if (argc > 4)
    {
        bin_cpu_capacity = stoi(argv[1]);
        bin_mem_capacity = stoi(argv[2]);
        density = stoi(argv[3]);
        graph = argv[4];
    }
    else
    {
        cout << "Usage: " << argv[0] << " <bin_cpu_capacity> <bin_mem_capacity> <density> <graph_class>" << endl;
        return -1;
    }

    string outfile(output_path + "densityTS_" + graph + "_" + to_string(bin_cpu_capacity) + "_" + to_string(bin_mem_capacity) + "_" + to_string(density) + ".csv");

    vector<string> list_algos = {
        /*"FF", "FFD-Degree",

        "FFD-Avg", "FFD-Max",
        "FFD-AvgExpo", "FFD-Surrogate",
        "FFD-ExtendedSum",

        "BFD-Avg", "BFD-Max",
        "BFD-AvgExpo", "BFD-Surrogate",
        "BFD-ExtendedSum",

        "WFD-Avg", "WFD-Max",
        "WFD-AvgExpo", "WFD-Surrogate",
        "WFD-ExtendedSum",

        "NCD-L2Norm",
        "NCD-DotProduct", "NCD-Fitness",
        "NCD-DotDivision",*/
    };

    vector<string> list_spread = {
        /*"SpreadWFD-Avg",
        "SpreadWFD-Max",
        "SpreadWFD-Surrogate",*/
        //"SpreadWFD-AvgExpo",
        //"SpreadWFD-ExtendedSum",
        "RefineWFD-Avg-2",
    };

    run_list_algos(input_path, outfile, list_algos, list_spread, bin_cpu_capacity, bin_mem_capacity, density, graph);

    std::cout << "Run successful!" << std::endl;
    return 0;
}


