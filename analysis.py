"""
proj2_analysis.py

Fill in the lists in the USER INPUT SECTION with your own timings.
Run with:  python proj2_analysis.py

Outputs:
- Printed tables for OMP & MPI speedup and efficiency
- Suggested "optimal" core/thread counts
- Optional weak-scaling tables if you provide those data
- Matplotlib plots that pop up and are also saved as PNG files
"""

import numpy as np
import matplotlib.pyplot as plt

# ===================== USER INPUT SECTION =====================

# Serial runtime (1 core, 1M-sample query)
serial_time = 0.0  # <-- put your measured serial time in seconds here, e.g. 2.34

# OMP: list of #threads tested and corresponding runtimes (same 1M problem)
omp_threads = [1, 2, 3, 4]          # example thread counts
omp_times   = [0.0, 0.0, 0.0, 0.0]  # <-- fill with your OMP runtimes in seconds

# MPI: list of #processes tested and corresponding runtimes (same 1M problem)
mpi_procs = [1, 2, 3, 4]            # example process counts
mpi_times = [0.0, 0.0, 0.0, 0.0]    # <-- fill with your MPI runtimes in seconds

# OPTIONAL (for part (c)): weak-scaling data, where problem size grows
# proportionally with #threads/#processes.
# Example: 1M, 2M, 3M, 4M samples when using 1, 2, 3, 4 threads.
weak_problem_sizes_omp = []   # e.g. [1_000_000, 2_000_000, 3_000_000, 4_000_000]
weak_times_omp         = []   # your OMP runtimes for those sizes

weak_problem_sizes_mpi = []   # e.g. [1_000_000, 2_000_000, 3_000_000, 4_000_000]
weak_times_mpi         = []   # your MPI runtimes for those sizes

# =================== END USER INPUT SECTION ===================


def compute_speedup_efficiency(serial_t, parallel_times, procs):
    parallel_times = np.array(parallel_times, dtype=float)
    procs = np.array(procs, dtype=float)
    speedup = serial_t / parallel_times
    efficiency = speedup / procs
    return speedup, efficiency


def estimate_parallel_fraction(speedup, procs):
    """
    Estimate parallel fraction f from Amdahl's law:

        S(p) = 1 / ((1 - f) + f / p)

    Solve for f:
        1/S = 1 - f + f/p
        f = (1 - 1/S) / (1 - 1/p)

    Only meaningful for p > 1.
    """
    S = np.array(speedup, dtype=float)
    p = np.array(procs, dtype=float)
    return (1.0 - 1.0 / S) / (1.0 - 1.0 / p)


def amdahl_speedup(p, f):
    p = np.array(p, dtype=float)
    return 1.0 / ((1.0 - f) + f / p)


def print_table(name, procs, times, speedup, efficiency):
    print(f"\n{name} results (1M-sample strong-scaling)")
    print(f"{'p':>3} {'time(s)':>12} {'speedup':>12} {'efficiency':>12}")
    for p, t, s, e in zip(procs, times, speedup, efficiency):
        print(f"{p:3d} {t:12.6f} {s:12.3f} {e:12.3f}")


def print_weak_scaling(name, procs, problem_sizes, times):
    if not problem_sizes or not times:
        print(f"\nNo weak-scaling data provided for {name}.")
        return

    procs = np.array(procs, dtype=int)
    problem_sizes = np.array(problem_sizes, dtype=float)
    times = np.array(times, dtype=float)

    print(f"\n{name} weak scaling (problem size grows with p)")
    print(f"{'p':>3} {'problem size':>15} {'time(s)':>12} {'N/p':>15}")
    for p, n, t in zip(procs, problem_sizes, times):
        print(f"{p:3d} {int(n):15d} {t:12.6f} {n/p:15.2f}")


def choose_optimal(procs, speedup, efficiency):
    procs = np.array(procs, dtype=int)
    speedup = np.array(speedup, dtype=float)
    efficiency = np.array(efficiency, dtype=float)

    p_best_speedup = int(procs[np.argmax(speedup)])
    p_best_eff = int(procs[np.argmax(efficiency)])

    return p_best_speedup, p_best_eff


def main():
    # ---- sanity checks ----
    if serial_time <= 0.0:
        raise ValueError("Please set 'serial_time' to a positive value.")

    if len(omp_threads) != len(omp_times):
        raise ValueError("omp_threads and omp_times must have the same length.")
    if len(mpi_procs) != len(mpi_times):
        raise ValueError("mpi_procs and mpi_times must have the same length.")

    # ---- OMP analysis ----
    omp_speedup, omp_eff = compute_speedup_efficiency(
        serial_time, omp_times, omp_threads
    )
    print_table("OMP", omp_threads, omp_times, omp_speedup, omp_eff)

    # estimate parallel fraction from runs with p > 1
    if len(omp_threads) > 1:
        p_gt1 = [p for p in omp_threads if p > 1]
        s_gt1 = [s for p, s in zip(omp_threads, omp_speedup) if p > 1]
        if p_gt1:
            f_omp = estimate_parallel_fraction(s_gt1, p_gt1)
            print(f"\nEstimated OMP parallel fraction f (Amdahl) ~ {np.mean(f_omp):.3f}")

    # ---- MPI analysis ----
    mpi_speedup, mpi_eff = compute_speedup_efficiency(
        serial_time, mpi_times, mpi_procs
    )
    print_table("MPI", mpi_procs, mpi_times, mpi_speedup, mpi_eff)

    if len(mpi_procs) > 1:
        p_gt1 = [p for p in mpi_procs if p > 1]
        s_gt1 = [s for p, s in zip(mpi_procs, mpi_speedup) if p > 1]
        if p_gt1:
            f_mpi = estimate_parallel_fraction(s_gt1, p_gt1)
            print(f"\nEstimated MPI parallel fraction f (Amdahl) ~ {np.mean(f_mpi):.3f}")

    # ---- Optimal core/thread choice (part (b)) ----
    omp_best_S, omp_best_E = choose_optimal(omp_threads, omp_speedup, omp_eff)
    mpi_best_S, mpi_best_E = choose_optimal(mpi_procs, mpi_speedup, mpi_eff)

    print("\n=== Suggested 'optimal' choices ===")
    print(f"OMP: max speedup at {omp_best_S} threads; "
          f"max efficiency at {omp_best_E} threads.")
    print(f"MPI: max speedup at {mpi_best_S} processes; "
          f"max efficiency at {mpi_best_E} processes.")

    # ---- Weak scaling (part (c)) ----
    print_weak_scaling("OMP", omp_threads, weak_problem_sizes_omp, weak_times_omp)
    print_weak_scaling("MPI", mpi_procs, weak_problem_sizes_mpi, weak_times_mpi)

    # ==================== PLOTS ====================

    # Strong scaling: speedup
    plt.figure()
    plt.plot(omp_threads, omp_speedup, marker='o', label="OMP")
    plt.plot(mpi_procs, mpi_speedup, marker='s', label="MPI")
    plt.xlabel("Number of threads / processes (p)")
    plt.ylabel("Observed speedup S(p)")
    plt.title("Speedup vs. p (strong scaling, 1M samples)")
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.savefig("speedup_strong_scaling.png")

    # Strong scaling: efficiency
    plt.figure()
    plt.plot(omp_threads, omp_eff, marker='o', label="OMP")
    plt.plot(mpi_procs, mpi_eff, marker='s', label="MPI")
    plt.xlabel("Number of threads / processes (p)")
    plt.ylabel("Efficiency E(p) = S(p)/p")
    plt.title("Efficiency vs. p (strong scaling, 1M samples)")
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.savefig("efficiency_strong_scaling.png")

    # Weak scaling plots (if data exist)
    if weak_problem_sizes_omp and weak_times_omp:
        plt.figure()
        plt.plot(omp_threads, weak_times_omp, marker='o')
        plt.xlabel("Number of threads (p)")
        plt.ylabel("Runtime (s)")
        plt.title("OMP weak scaling runtime")
        plt.grid(True)
        plt.tight_layout()
        plt.savefig("omp_weak_scaling_runtime.png")

    if weak_problem_sizes_mpi and weak_times_mpi:
        plt.figure()
        plt.plot(mpi_procs, weak_times_mpi, marker='o')
        plt.xlabel("Number of processes (p)")
        plt.ylabel("Runtime (s)")
        plt.title("MPI weak scaling runtime")
        plt.grid(True)
        plt.tight_layout()
        plt.savefig("mpi_weak_scaling_runtime.png")

    plt.show()


if __name__ == "__main__":
    main()
