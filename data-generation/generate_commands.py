#!/usr/bin/env python3
import csv
import sys
import uuid
import random
import math
from dataclasses import dataclass
from datetime import datetime, timedelta

# -----------------------------
# Configuration
# -----------------------------

# Controls how fast high risk levels become rare:
# base weight ~ exp(-RISK_DECAY * (risk_level - 1))
RISK_DECAY = 0.9
MAX_USERS = 2000  # safety cap for huge datasets

# Exponential-like decay over risk_level = 1..5
BASE_RISK_WEIGHTS = {
    level: math.exp(-RISK_DECAY * (level - 1))
    for level in range(1, 6)
}

# Threat levels (internal only, 0..4) ~ exponentially rarer for higher levels
THREAT_LEVEL_WEIGHTS = [1.0, 0.3, 0.08, 0.02, 0.005]

SHELL_TYPES = [
    ("bash", 0.7),
    ("zsh", 0.2),
    ("fish", 0.05),
    ("sh", 0.05),
]

HOSTNAMES = [
    "labpc-01", "labpc-02", "labpc-03", "labpc-04", "labpc-05",
    "labpc-06", "labpc-07", "labpc-08", "labpc-09", "labpc-10",
    "vm-ubuntu-01", "vm-ubuntu-02",
    "cs-lab-01", "cs-lab-02",
    "personal-laptop", "remote-ssh-01",
]


@dataclass
class User:
    user_id: int
    user_name: str
    shell_type: str
    home_dir: str
    threat_level: int  # 0..4, internal only
    activity_weight: float  # controls how often this user appears


@dataclass
class CommandTemplate:
    base_command: str
    risk_level: int          # 1..5
    sudo_probability: float  # chance to prefix with "sudo "
    patterns: list           # list of format strings using context keys
    base_weight: float       # global sampling weight before user scaling


# -----------------------------
# Command templates
# -----------------------------

def build_command_templates():
    """Return a list of CommandTemplate objects."""
    cmds = []

    def add(base, risk, sudo_p, patterns):
        cmds.append(
            CommandTemplate(
                base_command=base,
                risk_level=risk,
                sudo_probability=sudo_p,
                patterns=patterns,
                base_weight=BASE_RISK_WEIGHTS[risk],
            )
        )

    # ---------- Risk level 1 (safe / normal usage) ----------
    # Original ones
    add("ls", 1, 0.0, [
        "ls",
        "ls -la",
        "ls {home}",
        "ls {proj}",
        "ls {proj}/src",
    ])
    add("pwd", 1, 0.0, [
        "pwd",
    ])
    add("cd", 1, 0.0, [
        "cd {home}",
        "cd {proj}",
        "cd {proj}/src",
        "cd {proj}/data",
    ])
    add("cat", 1, 0.0, [
        "cat {file_txt}",
        "cat {proj}/{file_py}",
    ])
    add("grep", 1, 0.0, [
        'grep "{pattern}" {file_txt}',
        'grep -R "{pattern}" {proj}',
    ])
    add("head", 1, 0.0, [
        "head {file_txt}",
        "head -n 20 {proj}/{file_py}",
    ])
    add("tail", 1, 0.0, [
        "tail {file_log}",
        "tail -f {file_log}",
    ])
    add("echo", 1, 0.0, [
        'echo "Hello, world"',
        'echo "DEBUG"',
    ])

    # Additional risk-1 templates
    add("whoami", 1, 0.0, [
        "whoami",
    ])
    add("history", 1, 0.0, [
        "history",
        "history | tail",
    ])
    add("man", 1, 0.0, [
        "man ls",
        "man grep",
        "man python",
    ])
    add("which", 1, 0.0, [
        "which python",
        "which gcc",
        "which git",
    ])
    add("id", 1, 0.0, [
        "id",
        "id {user_name}",
    ])
    add("date", 1, 0.0, [
        "date",
    ])
    add("df", 1, 0.0, [
        "df -h",
        "df -h {home}",
    ])
    add("free", 1, 0.0, [
        "free -h",
    ])
    add("uname", 1, 0.0, [
        "uname -a",
        "uname -r",
    ])
    add("top", 1, 0.0, [
        "top -b -n 1",
    ])
    add("wc", 1, 0.0, [
        "wc -l {file_txt}",
        "wc -w {file_txt}",
    ])
    add("find", 1, 0.0, [
        "find {proj} -maxdepth 1 -type f",
        'find {proj} -name "*.py"',
    ])
    add("less", 1, 0.0, [
        "less {file_txt}",
        "less {file_log}",
    ])
    add("more", 1, 0.0, [
        "more {file_txt}",
    ])
    add("env", 1, 0.0, [
        "env",
    ])
    add("printenv", 1, 0.0, [
        "printenv",
        "printenv PATH",
    ])
    add("clear", 1, 0.0, [
        "clear",
    ])
    add("alias", 1, 0.0, [
        "alias",
        'alias ll="ls -la"',
    ])
    add("unalias", 1, 0.0, [
        "unalias ll",
    ])
    add("hostname", 1, 0.0, [
        "hostname",
    ])
    add("groups", 1, 0.0, [
        "groups",
        "groups {user_name}",
    ])
    add("uptime", 1, 0.0, [
        "uptime",
    ])
    add("stat", 1, 0.0, [
        "stat {file_txt}",
        "stat {proj}/{file_py}",
    ])
    add("ls", 1, 0.0, [
        "ls -lh",
        "ls --color=auto",
    ])

    # ---------- Risk level 2 (normal dev / potentially impactful) ----------
    # Original ones
    add("touch", 2, 0.0, [
        "touch {file_txt}",
        "touch {proj}/{file_py}",
    ])
    add("mkdir", 2, 0.0, [
        "mkdir {proj}/build",
        "mkdir -p {proj}/results",
    ])
    add("rm", 2, 0.0, [
        "rm {file_txt}",
        "rm {proj}/{file_py}",
    ])
    add("nano", 2, 0.0, [
        "nano {file_txt}",
        "nano {proj}/{file_py}",
    ])
    add("vim", 2, 0.0, [
        "vim {file_txt}",
        "vim {proj}/{file_py}",
    ])
    add("python", 2, 0.0, [
        "python {file_py}",
        "python {proj}/{file_py}",
        "python -m pytest",
    ])
    add("pip", 2, 0.1, [
        "pip install {pkg}",
        "pip install --user {pkg}",
    ])
    add("git", 2, 0.0, [
        "git status",
        "git pull",
        "git checkout {branch}",
        'git commit -am "{commit_msg}"',
    ])
    add("ssh", 2, 0.0, [
        "ssh {remote_host}",
        "ssh {user_name}@{remote_host}",
    ])

    # Additional risk-2 templates
    add("cp", 2, 0.0, [
        "cp {file_txt} {file_txt}.bak",
        "cp {proj}/{file_py} {proj}/{file_py}.bak",
    ])
    add("mv", 2, 0.0, [
        "mv {file_txt} {file_txt}.old",
        "mv {proj}/{file_py} {proj}/old_{file_py}",
    ])
    add("rsync", 2, 0.1, [
        "rsync -av {proj}/ {proj}/backup/",
        "rsync -av {proj}/ user@{remote_host}:{proj}/",
    ])
    add("scp", 2, 0.0, [
        "scp {file_txt} {user_name}@{remote_host}:~/",
        "scp -r {proj} {user_name}@{remote_host}:~/projects/",
    ])
    add("curl", 2, 0.0, [
        "curl https://example.com",
        "curl -O https://example.com/file.txt",
    ])
    add("wget", 2, 0.0, [
        "wget https://example.com/file.txt",
        "wget -qO- https://example.com",
    ])
    add("tar", 2, 0.0, [
        "tar -czf {proj}/archive.tar.gz {proj}",
        "tar -xzf archive.tar.gz",
    ])
    add("zip", 2, 0.0, [
        "zip -r project.zip {proj}",
    ])
    add("unzip", 2, 0.0, [
        "unzip project.zip",
    ])
    add("ln", 2, 0.0, [
        "ln -s {proj}/{file_py} {home}/{file_py}",
        "ln -s {proj} {home}/proj_link",
    ])
    add("make", 2, 0.0, [
        "make",
        "make test",
    ])
    add("cmake", 2, 0.0, [
        "cmake .",
        "cmake ..",
    ])
    add("jupyter", 2, 0.0, [
        "jupyter notebook",
        "jupyter lab",
    ])
    add("conda", 2, 0.1, [
        "conda activate base",
        "conda create -n env python=3.11",
    ])
    add("virtualenv", 2, 0.0, [
        "virtualenv venv",
        "virtualenv -p python3 venv",
    ])
    add("npm", 2, 0.0, [
        "npm install",
        "npm run build",
    ])
    add("node", 2, 0.0, [
        "node {proj}/{file_js}",
    ])
    add("javac", 2, 0.0, [
        "javac Main.java",
    ])
    add("java", 2, 0.0, [
        "java Main",
    ])
    add("gcc", 2, 0.0, [
        "gcc main.c -o main",
    ])
    add("g++", 2, 0.0, [
        "g++ main.cpp -o main",
    ])
    add("cargo", 2, 0.0, [
        "cargo build",
        "cargo test",
    ])
    add("go", 2, 0.0, [
        "go build",
        "go test ./...",
    ])
    add("R", 2, 0.0, [
        "Rscript analysis.R",
    ])
    add("julia", 2, 0.0, [
        "julia script.jl",
    ])
    add("matlab", 2, 0.0, [
        "matlab -batch \"run('script.m')\"",
    ])
    add("ssh-keygen", 2, 0.0, [
        "ssh-keygen -t rsa -b 4096",
    ])

    # ---------- Risk level 3 (more advanced / admin-ish) ----------
    # Original ones
    add("docker", 3, 0.1, [
        "docker ps",
        "docker images",
        "docker run -it {container} /bin/bash",
        "docker run -p {port}:{port} {container}",
    ])
    add("rm", 3, 0.2, [
        "rm -rf {proj}/build",
        "rm -rf {proj}/.pytest_cache",
    ])
    add("chmod", 3, 0.1, [
        "chmod +x {script_sh}",
        "chmod 600 {file_txt}",
    ])
    add("chown", 3, 0.3, [
        "chown {user_name}:{user_name} {proj}",
        "chown -R {user_name}:{user_name} {proj}",
    ])
    add("systemctl", 3, 0.8, [
        "systemctl status",
        "systemctl status ssh",
    ])
    add("ps", 3, 0.0, [
        "ps aux | grep python",
        "ps -ef | grep {pattern}",
    ])

    # Additional risk-3 templates
    add("kill", 3, 0.0, [
        "kill -9 1234",
        "kill 1234",
    ])
    add("killall", 3, 0.0, [
        "killall python",
        "killall -9 java",
    ])
    add("crontab", 3, 0.5, [
        "crontab -l",
        "crontab -e",
    ])
    add("service", 3, 0.7, [
        "service ssh status",
        "service apache2 status",
    ])
    add("mount", 3, 0.5, [
        "mount",
        "mount /dev/sdb1 /mnt",
    ])
    add("umount", 3, 0.5, [
        "umount /mnt",
    ])
    add("journalctl", 3, 0.0, [
        "journalctl -xe",
        "journalctl -u ssh",
    ])
    add("tcpdump", 3, 0.5, [
        "tcpdump -i eth0",
        "tcpdump -i eth0 port 22",
    ])
    add("ifconfig", 3, 0.0, [
        "ifconfig",
    ])
    add("ip", 3, 0.0, [
        "ip a",
        "ip r",
    ])
    add("netstat", 3, 0.0, [
        "netstat -tulpn",
    ])
    add("nmap", 3, 0.0, [
        "nmap localhost",
        "nmap -sV localhost",
    ])
    add("ufw", 3, 0.7, [
        "ufw status",
    ])
    add("htop", 3, 0.0, [
        "htop",
    ])
    add("nice", 3, 0.0, [
        "nice -n 10 python {file_py}",
    ])
    add("renice", 3, 0.0, [
        "renice 10 -p 1234",
    ])
    add("chattr", 3, 0.7, [
        "chattr +i {file_txt}",
        "chattr -i {file_txt}",
    ])
    add("sysctl", 3, 0.7, [
        "sysctl -a",
        "sysctl net.ipv4.ip_forward",
    ])

    # ---------- Risk level 4 (high-risk admin) ----------
    # Original ones
    add("iptables", 4, 0.95, [
        "iptables -L",
        "iptables -F",
    ])
    add("useradd", 4, 0.95, [
        "useradd testuser",
        "useradd -m projectuser",
    ])
    add("userdel", 4, 0.95, [
        "userdel testuser",
        "userdel -r projectuser",
    ])
    add("apt-get", 4, 0.95, [
        "apt-get update",
        "apt-get install {pkg}",
    ])
    add("systemctl", 4, 0.98, [
        "systemctl stop ssh",
        "systemctl restart ssh",
    ])

    # Additional risk-4 templates
    add("ufw", 4, 0.95, [
        "ufw enable",
        "ufw allow 22",
        "ufw deny 22",
    ])
    add("adduser", 4, 0.95, [
        "adduser student",
    ])
    add("deluser", 4, 0.95, [
        "deluser student",
        "deluser --remove-home student",
    ])
    add("visudo", 4, 0.95, [
        "visudo",
    ])
    add("passwd", 4, 0.95, [
        "passwd {user_name}",
        "passwd root",
    ])
    add("groupadd", 4, 0.95, [
        "groupadd devs",
    ])
    add("groupdel", 4, 0.95, [
        "groupdel devs",
    ])
    add("service", 4, 0.98, [
        "service ssh stop",
        "service ssh restart",
    ])
    add("shutdown", 4, 0.99, [
        "shutdown -r now",
        "shutdown -h now",
    ])
    add("reboot", 4, 0.99, [
        "reboot",
    ])
    add("halt", 4, 0.99, [
        "halt",
    ])
    add("mount", 4, 0.95, [
        "mount /dev/sda1 /mnt",
    ])
    add("umount", 4, 0.95, [
        "umount /dev/sda1",
    ])
    add("lvremove", 4, 0.99, [
        "lvremove -f vg0/lv0",
    ])
    add("vgremove", 4, 0.99, [
        "vgremove -f vg0",
    ])

    # ---------- Risk level 5 (extremely destructive) ----------
    # Original ones
    add("rm", 5, 0.99, [
        "rm -rf /",
        "rm -rf /home/*",
        "rm -rf /var/log/*",
    ])
    add("mkfs", 5, 0.99, [
        "mkfs.ext4 /dev/sda1",
        "mkfs.xfs /dev/sdb",
    ])
    add("fdisk", 5, 0.99, [
        "fdisk /dev/sda",
        "fdisk /dev/nvme0n1",
    ])

    # Additional risk-5 templates
    add("dd", 5, 0.99, [
        "dd if=/dev/zero of=/dev/sda bs=1M",
        "dd if=/dev/random of=/dev/sdb bs=4K",
    ])
    add("shred", 5, 0.99, [
        "shred -n 3 -z /dev/sda",
        "shred -u {file_txt}",
    ])
    add("rm", 5, 0.99, [
        "rm -rf ~",
        "rm -rf /etc/*",
    ])
    add("chmod", 5, 0.99, [
        "chmod -R 000 /",
    ])
    add("chown", 5, 0.99, [
        "chown -R nobody:nogroup /",
    ])
    add(":(){:|:&};:", 5, 0.99, [
        # note: braces are escaped for Python .format
        ":(){{ :|:& }};:  # fork bomb",
    ])
    add("mkfs", 5, 0.99, [
        "mkfs.ext4 /dev/nvme0n1",
    ])
    add("wipefs", 5, 0.99, [
        "wipefs -a /dev/sda",
    ])
    add("lvremove", 5, 0.99, [
        "lvremove -f vg0/*",
    ])

    return cmds


COMMAND_TEMPLATES = build_command_templates()


# -----------------------------
# Helpers
# -----------------------------

def weighted_choice(pairs):
    """pairs: list of (value, weight). Returns a chosen value."""
    values, weights = zip(*pairs)
    return random.choices(values, weights=weights, k=1)[0]


def generate_users(num_events):
    """
    Generate a user population. The number of users scales with sqrt(num_events),
    providing more users for larger datasets while keeping per-user activity realistic.
    """
    num_users = int(max(10, min(MAX_USERS, (num_events ** 0.5) * 2)))
    users = []

    for i in range(num_users):
        user_id = 1000 + i
        # simple username pattern like student1000, student1001, ...
        user_name = f"student{user_id}"
        shell_type = weighted_choice(SHELL_TYPES)

        home_dir = f"/home/{user_name}"

        threat_level = random.choices(
            population=list(range(5)),
            weights=THREAT_LEVEL_WEIGHTS,
            k=1,
        )[0]

        # Activity roughly lognormal, scaled slightly by threat level
        base_activity = random.lognormvariate(0.0, 1.0)
        activity_weight = base_activity * (1.0 + 0.3 * threat_level)

        users.append(User(
            user_id=user_id,
            user_name=user_name,
            shell_type=shell_type,
            home_dir=home_dir,
            threat_level=threat_level,
            activity_weight=activity_weight,
        ))

    return users


def random_timestamp_last_year():
    """Return ISO-8601 timestamp within the last ~calendar year."""
    now = datetime.utcnow()
    start = now - timedelta(days=365)
    delta_seconds = (now - start).total_seconds()
    offset = random.random() * delta_seconds
    ts = start + timedelta(seconds=offset)
    # ISO 8601 with milliseconds and Z suffix
    return ts.strftime("%Y-%m-%dT%H:%M:%S.%f")[:-3] + "Z"


def random_working_directory(user):
    """Sample a plausible working directory for a user."""
    subdirs = [
        "",
        "projects",
        "projects/cs101",
        "projects/cs201",
        "projects/research",
        "Downloads",
        "Desktop",
        ".config",
        "Documents",
    ]
    choice = random.choice(subdirs + ["/tmp", "/var/log", "/etc"])
    if choice.startswith("/"):
        return choice
    if choice == "":
        return user.home_dir
    return f"{user.home_dir}/{choice}"


def build_context(user):
    """Context values used to format command patterns."""
    proj = f"{user.home_dir}/projects/cs{random.randint(101, 499)}"
    return {
        "home": user.home_dir,
        "proj": proj,
        "file_py": f"main{random.randint(0, 5)}.py",
        "file_txt": f"notes{random.randint(0, 9)}.txt",
        "file_log": f"app{random.randint(0, 3)}.log",
        "file_js": f"app{random.randint(0, 3)}.js",
        "pattern": random.choice(["TODO", "ERROR", "WARNING", "fixme", "BUG"]),
        "container": random.choice(["ubuntu:20.04", "python:3.11", "postgres:15", "nginx:latest"]),
        "port": random.choice(["8000", "8080", "3000", "5432"]),
        "pkg": random.choice(["numpy", "pandas", "torch", "django", "flask", "matplotlib"]),
        "branch": random.choice(["main", "dev", "feature-x", "bugfix-y"]),
        "commit_msg": random.choice(["wip", "fix bug", "add feature", "update tests"]),
        "remote_host": random.choice(["login.cluster.edu", "github.com", "gitlab.com"]),
        "script_sh": random.choice(["run.sh", "start.sh", "deploy.sh"]),
        "user_name": user.user_name,
    }


def choose_command_template_for_user(user):
    """
    Choose a command template, skewed so that:
    - High risk commands are globally rare due to BASE_RISK_WEIGHTS (exp-like).
    - Higher threat users are somewhat more likely to use riskier commands.
    """
    weights = []
    for tmpl in COMMAND_TEMPLATES:
        # Start from base weight, which already decays exponentially with risk_level
        w = tmpl.base_weight
        # Scale towards riskier commands for higher threat users
        risk_offset = tmpl.risk_level - 1
        if risk_offset > 0 and user.threat_level > 0:
            w *= (1.0 + 0.4 * user.threat_level * risk_offset)
        weights.append(w)

    idx = random.choices(range(len(COMMAND_TEMPLATES)), weights=weights, k=1)[0]
    return COMMAND_TEMPLATES[idx]


def build_raw_command(user, template):
    """Create a concrete raw_command string from a template for a given user."""
    ctx = build_context(user)
    pattern = random.choice(template.patterns)

    # Safely apply .format; if something goes wrong, fall back to literal pattern
    try:
        core = pattern.format(**ctx)
    except Exception:
        core = pattern

    # sudo
    sudo_used = False
    if random.random() < template.sudo_probability:
        core = "sudo " + core
        sudo_used = True

    # Occasionally, chain a harmless follow-up command with && or |
    if random.random() < 0.08 and template.risk_level <= 3:
        follow = random.choice([
            'echo "done"',
            "pwd",
            "ls",
            'echo "OK"',
        ])
        joiner = random.choice([" && ", " | "])
        core = core + joiner + follow

    return core, sudo_used


def sample_exit_code(risk_level):
    """
    Sample an exit code: mostly 0, with higher risk commands more likely to fail.
    """
    base_fail_probs = {
        1: 0.03,
        2: 0.06,
        3: 0.10,
        4: 0.16,
        5: 0.22,
    }
    if random.random() >= base_fail_probs.get(risk_level, 0.1):
        return 0
    # some common error codes
    return random.choice([1, 2, 126, 127, 130])


def choose_host():
    return random.choice(HOSTNAMES)


def generate_rows(num_rows):
    """Yield dictionaries matching the schema for num_rows synthetic command events."""
    users = generate_users(num_rows)
    user_weights = [u.activity_weight for u in users]

    for _ in range(num_rows):
        user = random.choices(users, weights=user_weights, k=1)[0]
        tmpl = choose_command_template_for_user(user)
        raw_command, sudo_used = build_raw_command(user, tmpl)

        yield {
            "command_id": str(uuid.uuid4()),
            "raw_command": raw_command,
            "base_command": tmpl.base_command,
            "shell_type": user.shell_type,
            "exit_code": sample_exit_code(tmpl.risk_level),
            "timestamp": random_timestamp_last_year(),
            "sudo_used": str(sudo_used).lower(),  # "true"/"false"
            "working_directory": random_working_directory(user),
            "user_id": user.user_id,
            "user_name": user.user_name,
            "host_name": choose_host(),
            "risk_level": tmpl.risk_level,
        }


def main(argv=None):
    if argv is None:
        argv = sys.argv

    if len(argv) < 2:
        print(f"Usage: {argv[0]} NUM_ROWS [OUTPUT_CSV]", file=sys.stderr)
        sys.exit(1)

    try:
        num_rows = int(argv[1])
        if num_rows <= 0:
            raise ValueError
    except ValueError:
        print("NUM_ROWS must be a positive integer.", file=sys.stderr)
        sys.exit(1)

    output_path = argv[2] if len(argv) >= 3 else "synthetic_commands.csv"

    fieldnames = [
        "command_id",
        "raw_command",
        "base_command",
        "shell_type",
        "exit_code",
        "timestamp",
        "sudo_used",
        "working_directory",
        "user_id",
        "user_name",
        "host_name",
        "risk_level",
    ]

    with open(output_path, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        for row in generate_rows(num_rows):
            writer.writerow(row)

    print(f"Wrote {num_rows} rows to {output_path}")


if __name__ == "__main__":
    main()
