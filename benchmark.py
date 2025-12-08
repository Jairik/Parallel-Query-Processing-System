import curses
import subprocess
import sys
import os
import time

def run_command(command, silent=False):
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
        # We don't raise here to allow the script to continue to cleanup

def draw_menu(stdscr, selected_row_idx, options):
    stdscr.clear()
    h, w = stdscr.getmaxyx()
    
    title = "Select Benchmark to Run (Use Arrow Keys + Enter)"
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

def main(stdscr):
    # Setup colors
    curses.start_color()
    curses.init_pair(1, curses.COLOR_BLACK, curses.COLOR_WHITE)
    curses.curs_set(0)

    options = ["Serial", "OMP", "MPI", "ALL", "Exit"]
    selected_row_idx = 0

    while True:
        draw_menu(stdscr, selected_row_idx, options)
        key = stdscr.getch()

        if key == curses.KEY_UP and selected_row_idx > 0:
            selected_row_idx -= 1
        elif key == curses.KEY_DOWN and selected_row_idx < len(options) - 1:
            selected_row_idx += 1
        elif key == curses.KEY_ENTER or key in [10, 13]:
            return options[selected_row_idx]

def run_benchmark():
    # Run make silently
    print("Building project... (this may take a moment)")
    run_command("make", silent=True)

    # Show Menu
    try:
        # curses.wrapper handles initialization and cleanup of curses
        selection = curses.wrapper(main)
    except Exception as e:
        print(f"Error in UI: {e}")
        # Ensure cleanup happens even if UI fails
        run_command("make clean", silent=True)
        return

    if selection == "Exit":
        print("Exiting...")
        run_command("make clean", silent=True)
        return

    print(f"\nRunning {selection} Benchmark...\n" + "="*40 + "\n")

    # Run Selected
    start_time = time.time()
    
    if selection == "Serial":
        run_command("make run")
    elif selection == "OMP":
        run_command("make run-omp")
    elif selection == "MPI":
        run_command("make run-mpi")
    elif selection == "ALL":
        print("--- Running Serial ---")
        run_command("make run")
        print("\n--- Running OMP ---")
        run_command("make run-omp")
        print("\n--- Running MPI ---")
        run_command("make run-mpi")

    end_time = time.time()
    print("\n" + "="*40)
    print(f"Total Benchmark Time: {end_time - start_time:.4f} seconds")

    # Run make clean silently
    print("Cleaning up build artifacts...")
    run_command("make clean", silent=True)
    print("Done.")

if __name__ == "__main__":
    run_benchmark()
