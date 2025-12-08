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
        return None, "No CSV files found in data-generation/"
    
    datasets.append("Exit")
    dataset = select_option(stdscr, "Select Dataset (Use Arrow Keys + Enter)", datasets)
    
    if dataset == "Exit":
        return "Exit", None

    # Select Benchmark Version
    benchmarks = ["Serial", "OMP", "MPI", "ALL", "Exit"]
    benchmark = select_option(stdscr, "Select Benchmark to Run", benchmarks)

    return benchmark, dataset

def run_benchmark():
    # Run make silently
    print("Building project... (this may take a moment)")
    run_command("make", silent=True)

    # Show Menu
    try:
        selection, dataset = curses.wrapper(main)
    except Exception as e:
        print(f"Error in UI: {e}")
        # run_command("make clean", silent=True)
        return

    if selection == "Exit" or selection is None:
        if dataset and dataset != "Exit":
             print(dataset) # Print error message if any
        print("Exiting...")
        # run_command("make clean", silent=True)
        return

    print(f"\nRunning {selection} Benchmark with dataset: {dataset}\n" + "="*60 + "\n")

    # Run Selected
    start_time = time.time() 
    
    # Pass dataset as ARGS
    # args = f'ARGS="{dataset}"'
    
    # Run the executable of the select benchmark, avoid make rules for overhead concerns
    if selection == "Serial":
        run_command(f"./QPESeq {dataset}")
    
    elif selection == "OMP":
        run_command(f"./QPEOMP {dataset}")
    
    elif selection == "MPI":
        run_command(f"./QPEMPI {dataset}")
    
    elif selection == "ALL":
        print("--- Running Serial ---")
        run_command(f"./QPESeq {dataset}")
        
        print("\n--- Running OMP ---")
        run_command(f"./QPEOMP {dataset}")
        
        print("\n--- Running MPI ---")
        run_command(f"./QPEMPI {dataset}")

    end_time = time.time()
    print("\n" + "="*60)
    print(f"Total Benchmark Time: {end_time - start_time:.4f} seconds")

    # # Run make clean silently
    # print("Cleaning up build artifacts...")
    # run_command("make clean", silent=True)
    # print("Done.")

if __name__ == "__main__":
    run_benchmark()
