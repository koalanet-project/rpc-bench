import os
import sys
import signal
import subprocess
import argparse
import time
import multiprocessing

args_parser = argparse.ArgumentParser()
args_parser.add_argument('server', metavar='server',
                         type=str, nargs='?', default='rdma0.danyang-06')
args_parser.add_argument(
    '--opt', type=str, nargs='?', default='', help="Options of rpc-bench")
args_parser.add_argument(
    '-u', action='store_true', help="Monitor CPU utilization at the client (default at the server)")

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
        envoy_cmd = "nohup envoy -c envoy/envoy.yaml >scripts/%s/envoy_server.log 2>&1 &" % opt.a
    server_script = '''
    cd ~/nfs/Developing/rpc-bench; %s;
    GLOG_minloglevel=2 nohup ./build/rpc-bench -a %s -r grpc -d %d -T %d >/dev/null 2>&1 &
    ''' % (envoy_cmd, opt.a, opt.d, opt.T)
    print('server envoy:', envoy_cmd)
    print('server:', server_script)
    ssh_cmd(args.server, server_script)
    time.sleep(2)


def run_client(args, opt):
    envoy_cmd = ""
    env = dict(os.environ, GLOG_minloglevel="2", GLOG_logtostderr="1")
    print(f'enable Envoy: {args.envoy}')
    if args.envoy:
        envoy_cmd = "nohup envoy -c envoy/envoy.yaml >scripts/%s/envoy_client.log 2>&1 &" % opt.a
        env = dict(env, http_proxy="http://127.0.0.1:10000/",
                   grpc_proxy="http://127.0.0.1:10000/")
    print('client envoy:', envoy_cmd)
    os.system(envoy_cmd)
    time.sleep(1)
    client_script = '''
    ./build/rpc-bench -a %s -r grpc -d %d -C %d -T %d -p %d -t %d --monitor-time=%d %s
    ''' % (opt.a, opt.d, opt.C, opt.T, opt.p, opt.t, opt.t, args.server)
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


def parse_cpus(raw_output, cpu_count):
    cpus = []
    for row in raw_output.split('\n'):
        line = row.split()
        utime = float(line[3]) * cpu_count
        stime = float(line[5]) * cpu_count
        soft = float(line[8]) * cpu_count
        non_idle = (100 - float(line[-1])) * cpu_count
        # cpus.append([utime, stime, soft])
        cpus.append([non_idle])
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

    cpus_srv = [sum(stat) for stat in cpus_srv]
    cpus_cli = [sum(stat) for stat in cpus_cli]
    return cpus_srv, cpus_cli
