#include <omp.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>

#define THREADS 8

class Sorter {
public:
    class Printer {
    public:
        explicit Printer(double completionTime, const std::string& label)
            : _completionTime(completionTime), _label(label) {}
        void print() const {
            std::cout << _label << ": " << _completionTime << " мс"
                      << std::endl;
        }

    private:
        double _completionTime;
        std::string _label;
    };

    static void sort(std::vector<int>& array) {
        bool sorted    = false;
        const int size = array.size();
        while (!sorted) {
            sorted = true;
            for (int i = 1; i < size - 1; i += 2) {
                if (array[i] < array[i + 1]) {
                    std::swap(array[i], array[i + 1]);
                    sorted = false;
                }
            }
            for (int i = 0; i < size - 1; i += 2) {
                if (array[i] < array[i + 1]) {
                    std::swap(array[i], array[i + 1]);
                    sorted = false;
                }
            }
        }
    }

    static void sort_parallel(std::vector<int>& array) {
        const int size = array.size();
        bool sorted    = false;
        while (!sorted) {
            bool swapped_odd = false;
#pragma omp parallel for reduction(|| : swapped_odd) schedule(static)
            for (int i = 1; i < size - 1; i += 2) {
                if (array[i] < array[i + 1]) {
                    std::swap(array[i], array[i + 1]);
                    swapped_odd = true;
                }
            }
            bool swapped_even = false;
#pragma omp parallel for reduction(|| : swapped_even) schedule(static)
            for (int i = 0; i < size - 1; i += 2) {
                if (array[i] < array[i + 1]) {
                    std::swap(array[i], array[i + 1]);
                    swapped_even = true;
                }
            }
            sorted = !(swapped_odd || swapped_even);
        }
    }

    static std::vector<int> generate(int n) {
        std::vector<int> result;
        for (int i = 0; i < n; i++) {
            result.push_back(rand() % 200 - 100);
        }
        return result;
    }

    static bool check(const std::vector<int>& a) {
        bool sorted = true;
#pragma omp parallel for reduction(&& : sorted)
        for (int i = 0; i < a.size() - 1; i++) {
            if (a[i] < a[i + 1]) {
                sorted = false;
            }
        }
        return sorted;
    }
};

int main() {
    omp_set_num_threads(THREADS);

    std::vector<int> vec  = Sorter::generate(10000);
    std::vector<int> copy = vec;

    auto start_serial = std::chrono::high_resolution_clock::now();
    Sorter::sort(vec);
    auto end_serial    = std::chrono::high_resolution_clock::now();
    double serial_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                             end_serial - start_serial)
                             .count();

    auto start_parallel = std::chrono::high_resolution_clock::now();
    Sorter::sort_parallel(copy);
    auto end_parallel = std::chrono::high_resolution_clock::now();
    double parallel_time =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            end_parallel - start_parallel)
            .count();

    assert(Sorter::check(vec));
    assert(Sorter::check(copy));

    Sorter::Printer(serial_time, "Последовательная сортировка").print();
    Sorter::Printer(parallel_time, "Параллельная сортировка").print();

    return 0;
}
