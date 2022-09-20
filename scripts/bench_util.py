import os
import sys
import signal
import subprocess
import argparse
import time
import datetime
import multiprocessing

args_parser = argparse.ArgumentParser()
args_parser.add_argument('server', metavar='server',
                         type=str, nargs='?', default='rdma0.danyang-06')
args_parser.add_argument('-c', type=str, nargs='?',
                         default='envoy/envoy.yaml', help='envoy config file')
args_parser.add_argument(
    '--opt', type=str, nargs='?', default='', help="Options of rpc-bench")


opt_parser = argparse.ArgumentParser()
opt_parser.add_argument('-a', type=str, nargs='?')
opt_parser.add_argument('-p', type=int, nargs='?', default=18000)
opt_parser.add_argument('-t', type=int, nargs='?', default=10)
opt_parser.add_argument('-C', type=int, nargs='?', default=32)
opt_parser.add_argument('-T', type=int, nargs='?', default=1)
opt_parser.add_argument('-d', type=int, nargs='?', default=32)


def ssh_cmd(server, cmd):
    with subprocess.Popen(f'ssh -o "StrictHostKeyChecking no" {server} "{cmd}"', shell=True,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE) as proc:
        out, err = proc.communicate()
        out = out.decode().strip()
        err = err.decode().strip()
    return err + out


def killall(args):
    cmd = "pkill -9 rpc-bench"
    os.system(cmd)
    ssh_cmd(args.server, cmd)

    cmd = "pkill -9 envoy"
    os.system(cmd)
    ssh_cmd(args.server, cmd)

    cmd = "pkill -9 mpstat"
    os.system(cmd)
    ssh_cmd(args.server, cmd)

    time.sleep(1)


def run_server(args, opt):
    envoy_cmd = ""
    print(f'enable Envoy: {args.envoy}')
    if args.envoy:
        envoy_cmd = f"nohup envoy -c {args.c} >scripts/{opt.a}/envoy_server.log 2>&1 &"
    server_script = f'''
    cd ~/nfs/Developing/rpc-bench; {envoy_cmd};
    GLOG_minloglevel=2 nohup ./build/rpc-bench -a {opt.a} -r grpc -d {opt.d} -T {opt.T} >/dev/null 2>&1 &
    '''
    print('server envoy:', envoy_cmd)
    print('server:', server_script)
    ssh_cmd(args.server, server_script)
    time.sleep(2)


def run_client(args, opt):
    envoy_cmd = ""
    env = dict(os.environ, GLOG_minloglevel="2", GLOG_logtostderr="1")
    print(f'enable Envoy: {args.envoy}')
    if args.envoy:
        envoy_cmd = f"nohup envoy -c {args.c} >scripts/{opt.a}/envoy_client.log 2>&1 &"
        env = dict(env, http_proxy="http://127.0.0.1:10000/",
                   grpc_proxy="http://127.0.0.1:10000/")
    print('client envoy:', envoy_cmd)
    os.system(envoy_cmd)
    time.sleep(1)
    client_script = f'''
    ./build/rpc-bench -a {opt.a} -r grpc -d {opt.d} -C {opt.C} -T {opt.T} -p {opt.p} -t {opt.t} --monitor-time={opt.t} {args.server}
    '''
    print('client:', client_script)
    with subprocess.Popen(client_script.split(), env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE) as proc:
        out, err = proc.communicate()
    out = out.decode().strip()
    err = err.decode().strip()
    return err + out


def run_cpu_monitor(args, tag):
    if args.envoy:
        tag = 'envoy_' + tag
    mpstat = "mpstat -P ALL -u 1  | grep --line-buffered all"

    fsrv = f"/tmp/rpc_bench_cpu_monitor_grpc_server_{tag}_{args.timestamp}"
    cmd = f"nohup sh -c '{mpstat}' >{fsrv} 2>/dev/null &"
    ssh_cmd(args.server, cmd)

    fcli = f"/tmp/rpc_bench_cpu_monitor_grpc_client_{tag}_{args.timestamp}"
    cmd = f"nohup sh -c '{mpstat}' >{fcli} 2>/dev/null &"
    os.system(cmd)

    return (fsrv, fcli)


timeformat = "%Y-%m-%dT%H:%M:%S"
now_timestamp = time.time()
localOffset = (datetime.datetime.fromtimestamp(
    now_timestamp) - datetime.datetime.utcfromtimestamp(now_timestamp)).total_seconds()
# Beijing: localOffset=28800


def toTimestamp(strtime, offset=localOffset):
    return int(time.mktime(time.strptime(strtime, timeformat))) + localOffset - offset
# Beijing: offset=28800
# toTimestamp(strtime): localtime->timestamp
# toTimestamp(strtime, 0): utc time->timestamp


def parse_timestamp(t1, t2):
    today = str(datetime.date.today())
    h, m, s = [int(x) for x in t1.split(':')]
    if t2 == 'PM':
        h += 12
    strtime = f"{today}T{h}:{m}:{s}"
    return toTimestamp(strtime)


def align_by_timestamp(base, seqs):  # seq must cover the base
    if len(base) == 0:
        return [], []
    basev = [v for ts, v in base]
    seqvs = []
    for seq in seqs:
        i = 0
        while i < len(seq):
            if seq[i][0] == base[0][0]:
                break
            i += 1
        j = len(seq) - 1
        while j > i:
            if seq[j][0] == base[-1][0]:
                break
            j -= 1
        seqv = [v for ts, v in seq[i:j + 1]]
        seqvs.append(seqv)
    return basev, seqvs


def parse_cpus(raw_output, cpu_count):
    cpus = []
    for row in raw_output.split('\n'):
        line = row.split()
        utime = float(line[3]) * cpu_count
        stime = float(line[5]) * cpu_count
        soft = float(line[8]) * cpu_count
        non_idle = (100 - float(line[-1])) * cpu_count
        ts = parse_timestamp(line[0], line[1])
        cpus.append((ts, [non_idle]))
        # cpus.append((ts, [utime, stime, soft]))
    return cpus


def stop_cpu_monitor(args, fname: tuple):
    killcmd = "pkill -9 mpstat"
    os.system(killcmd)
    ssh_cmd(args.server, killcmd)
    fsrv, fcli = fname

    with open(fcli, "r") as fout:
        raw_output = fout.read().strip()
    cpu_count = multiprocessing.cpu_count()
    cpus_cli = parse_cpus(raw_output, cpu_count)

    raw_output = ssh_cmd(args.server, f"cat {fsrv}")
    cpucmd = "python3 -c 'from multiprocessing import cpu_count; print(cpu_count())'"
    cpu_count = int(ssh_cmd(args.server, cpucmd))
    cpus_srv = parse_cpus(raw_output, cpu_count)

    return (cpus_srv, cpus_cli)


def merge_mpstat(mpstat_srv, mpstat_cli, length):
    cpus_srv = mpstat_srv[-1 - length:-1]
    cpus_cli = mpstat_cli[-1 - length:-1]
    print("mpstat server", cpus_srv)
    print("mpstat client", cpus_cli)

    cpus_srv = [sum(stat) for ts, stat in cpus_srv]
    cpus_cli = [sum(stat) for ts, stat in cpus_cli]
    return cpus_srv, cpus_cli
