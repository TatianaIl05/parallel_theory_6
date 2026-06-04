#include <boost/program_options.hpp>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

namespace po = boost::program_options;


void init_grid(double* u, double* u_new, int nx, int ny) {
    const double TL = 10.0;
    const double TR = 20.0;
    const double BR = 30.0;
    const double BL = 20.0;
    
    const int size = nx * ny;
    
    for (int k = 0; k < size; ++k) {
        u[k] = 0.0;
        u_new[k] = 0.0;
    }
    
    for (int j = 0; j < nx; ++j) {
        double t = static_cast<double>(j) / (nx - 1);
        u[0 * nx + j] = TL * (1.0 - t) + TR * t;
        u_new[0 * nx + j] = u[0 * nx + j];
    }
    
    for (int j = 0; j < nx; ++j) {
        double t = static_cast<double>(j) / (nx - 1);
        u[(ny - 1) * nx + j] = BL * (1.0 - t) + BR * t;
        u_new[(ny - 1) * nx + j] = u[(ny - 1) * nx + j];
    }
    
    for (int i = 1; i < ny - 1; ++i) {
        double t = static_cast<double>(i) / (ny - 1);
        u[i * nx + 0] = TL * (1.0 - t) + BL * t;
        u_new[i * nx + 0] = u[i * nx + 0];
    }
    
    for (int i = 1; i < ny - 1; ++i) {
        double t = static_cast<double>(i) / (ny - 1);
        u[i * nx + (nx - 1)] = TR * (1.0 - t) + BR * t;
        u_new[i * nx + (nx - 1)] = u[i * nx + (nx - 1)];
    }
}


void save_result(const std::string& filename, const double* u, int nx, int ny) {
    std::ofstream out(filename);
    out << std::setprecision(12);
    for (int i = 0; i < ny; ++i) {
        for (int j = 0; j < nx; ++j) {
            out << u[i * nx + j];
            if (j + 1 < nx) out << ' ';
        }
        out << '\n';
    }
}


void print_grid(const double* u, int nx, int ny) {
    std::cout << std::fixed << std::setprecision(4);
    for (int i = 0; i < ny; ++i) {
        for (int j = 0; j < nx; ++j) {
            std::cout << std::setw(8) << u[i * nx + j];
        }
        std::cout << '\n';
    }
}


struct Config {
    int nx = 128;
    int ny = 128;
    double eps = 1e-6;
    int max_iter = 1000000;
    std::string out_file = "result.dat";
};

Config parse_args(int argc, char** argv) {
    Config cfg;
    
    po::options_description desc("Опции");
    desc.add_options()
        ("help,h", "Помощь")
        ("nx", po::value<int>(&cfg.nx)->default_value(128), "Размер по X")
        ("ny", po::value<int>(&cfg.ny)->default_value(128), "Размер по Y")
        ("eps,e", po::value<double>(&cfg.eps)->default_value(1e-6), "Точность")
        ("max-iter,i", po::value<int>(&cfg.max_iter)->default_value(1000000), "Макс. итераций")
        ("out,o", po::value<std::string>(&cfg.out_file)->default_value("result.dat"), "Выходной файл");
    
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    
    if (vm.count("help")) {
        std::cout << desc << std::endl;
        exit(0);
    }
    
    return cfg;
}


int main(int argc, char** argv) {
    Config cfg = parse_args(argc, argv);
    
    const int N = cfg.nx;
    const int size = cfg.nx * cfg.ny;
    
    std::cout << "Сетка: " << cfg.ny << " x " << cfg.nx << std::endl;
    std::cout << "Точность: " << cfg.eps << std::endl;
    std::cout << "Макс. итераций: " << cfg.max_iter << std::endl;
    
    std::unique_ptr<double[]> u(new double[size]);
    std::unique_ptr<double[]> u_new(new double[size]);
    
    init_grid(u.get(), u_new.get(), cfg.nx, cfg.ny);
    
    double* cur = u.get();
    double* nxt = u_new.get();
    int iter = 0;
    double error = 0.0;
    
    auto start_time = std::chrono::steady_clock::now();
    
    #pragma acc data copy(cur[0:size], nxt[0:size])
    {
        while (iter < cfg.max_iter) {
            error = 0.0;
            
            #pragma acc parallel loop collapse(2) reduction(max:error) present(cur[0:size], nxt[0:size])
            for (int i = 1; i < cfg.ny - 1; ++i) {
                for (int j = 1; j < cfg.nx - 1; ++j) {
                    double val = 0.25 * (cur[(i + 1) * N + j] +
                                         cur[(i - 1) * N + j] +
                                         cur[i * N + (j + 1)] +
                                         cur[i * N + (j - 1)]);
                    
                    nxt[i * N + j] = val;
                    
                    double diff = fabs(val - cur[i * N + j]);
                    if (diff > error) error = diff;
                }
            }
            
            std::swap(cur, nxt);
            iter++;
            
            if (error < cfg.eps) break;
        }
    }
    
    auto end_time = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(end_time - start_time).count();
    
    std::cout << "\nРезультаты: " << std::endl;
    std::cout << "Итераций: " << iter << std::endl;
    std::cout << "Ошибка: " << error << std::endl;
    std::cout << "Время: " << elapsed << " сек" << std::endl;
    
    save_result(cfg.out_file, cur, cfg.nx, cfg.ny);
    std::cout << "Результат сохранен в " << cfg.out_file << std::endl;
    
    if (cfg.nx == cfg.ny && (cfg.nx == 10 || cfg.nx == 13)) {
        std::cout << "\nФинальная сетка (" << cfg.nx << "x" << cfg.ny << "):" << std::endl;
        print_grid(cur, cfg.nx, cfg.ny);
    }
    
    return 0;
}
