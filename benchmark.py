import curses
import subprocess
import sys
import os
import time

def run_command(command, silent=False, dataset=None):
    """Runs a shell command."""
    try:
        if silent:
            subprocess.run(command, shell=True, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        else:
            subprocess.run(command, shell=True, check=True)
    except subprocess.CalledProcessError as e:
        if not silent:
            print(f"Error running command: {command}")
            print(f"Exit code: {e.returncode}")

def draw_menu(stdscr, selected_row_idx, options, title):
    stdscr.clear()
    h, w = stdscr.getmaxyx()
    
    stdscr.addstr(h//2 - len(options)//2 - 2, w//2 - len(title)//2, title)

    for idx, row in enumerate(options):
        x = w//2 - len(row)//2
        y = h//2 - len(options)//2 + idx
        if idx == selected_row_idx:
            stdscr.attron(curses.color_pair(1))
            stdscr.addstr(y, x, row)
            stdscr.attroff(curses.color_pair(1))
        else:
            stdscr.addstr(y, x, row)

    stdscr.refresh()

def select_option(stdscr, title, options):
    selected_row_idx = 0
    while True:
        draw_menu(stdscr, selected_row_idx, options, title)
        key = stdscr.getch()

        if key == curses.KEY_UP and selected_row_idx > 0:
            selected_row_idx -= 1
        elif key == curses.KEY_DOWN and selected_row_idx < len(options) - 1:
            selected_row_idx += 1
        elif key == curses.KEY_ENTER or key in [10, 13]:
            return options[selected_row_idx]

def get_datasets():
    data_dir = "data-generation"
    if not os.path.exists(data_dir):
        return []
    files = [f for f in os.listdir(data_dir) if f.endswith(".csv")]
    files.sort()
    # Return relative paths
    return [os.path.join(data_dir, f) for f in files]

def main(stdscr):
    # Setup colors
    curses.start_color()
    curses.init_pair(1, curses.COLOR_BLACK, curses.COLOR_WHITE)
    curses.curs_set(0)

    # Select Dataset
    datasets = get_datasets()
    if not datasets:
        return None, "No CSV files found in data-generation/", None
    
    datasets.append("Exit")
    dataset = select_option(stdscr, "Select Dataset (Use Arrow Keys + Enter)", datasets)
    
    if dataset == "Exit":
        return "Exit", None, None

    # Select Benchmark Version
    benchmarks = ["Serial", "OMP", "MPI", "ALL", "Exit"]
    benchmark = select_option(stdscr, "Select Benchmark to Run", benchmarks)

    if benchmark == "Exit":
        return "Exit", None, None

    # Select Cores
    cores = None
    if benchmark in ["OMP", "MPI", "ALL"]:
        try:
            num_cpus = os.cpu_count()
            if num_cpus is None: num_cpus = 4
        except:
            num_cpus = 4
            
        core_options = ["ALL"] + [str(i) for i in range(1, num_cpus + 1)]
        cores = select_option(stdscr, "Select Number of Cores", core_options)

    return benchmark, dataset, cores

def run_benchmark():
    # Run make silently
    print("Building project... (this may take a moment)")
    run_command("make", silent=True)

    # Show Menu
    try:
        result = curses.wrapper(main)
        if not result: return
        selection, dataset, cores = result
    except Exception as e:
        print(f"Error in UI: {e}")
        return

    if selection is None:
        if dataset:
             print(dataset) # Print error message
        return

    if selection == "Exit":
        print("Exiting...")
        return

    print(f"\nRunning {selection} Benchmark with dataset: {dataset}")
    if cores:
        print(f"Cores: {cores}")
    print("="*60 + "\n")

    # Determine core count
    omp_prefix = ""
    mpi_prefix = ""
    
    if cores:
        count = 0
        if cores == "ALL":
            count = os.cpu_count() or 4
        else:
            count = int(cores)
            
        omp_prefix = f"OMP_NUM_THREADS={count} "
        mpi_prefix = f"mpirun -np {count} "

    # Run Selected
    start_time = time.time() 
    
    if selection == "Serial":
        run_command(f"./QPESeq {dataset}")
    
    elif selection == "OMP":
        run_command(f"{omp_prefix}./QPEOMP {dataset}")
    
    elif selection == "MPI":
        run_command(f"{mpi_prefix}./QPEMPI {dataset}")
    
    elif selection == "ALL":
        print("--- Running Serial ---")
        run_command(f"./QPESeq {dataset}")
        
        print("\n--- Running OMP ---")
        run_command(f"{omp_prefix}./QPEOMP {dataset}")
        
        print("\n--- Running MPI ---")
        run_command(f"{mpi_prefix}./QPEMPI {dataset}")

    end_time = time.time()
    print("\n" + "="*60)
    print(f"Total Benchmark Time: {end_time - start_time:.4f} seconds")

if __name__ == "__main__":
    run_benchmark()
