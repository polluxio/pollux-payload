#!/usr/bin/env python

# SPDX-FileCopyrightText: 2023 Pollux authors <https://github.com/polluxio/pollux-payload/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0

import os
import shutil
import sys
import argparse
import datetime
import subprocess
import yaml
import logging

def create_pollux_configuration(path, args, nb_parts) -> None:
    if args.mode == 'local':
        executor = 'local'
    elif args.mode == 'qarnot':
        executor = 'qarnot-cluster'
    elif args.mode == 'local_docker':
        executor = 'docker'
      
    f = open(path, "w")
    f.write('synchronized: true\n')
    f.write('executor: ' + executor + '\n')
    f.write('payload: ' + args.payload + '\n')
    f.write('port: 50000' + '\n')
    f.write('payloads_nb: ' + str(nb_parts) + '\n')
    f.write('verbosity: ' + args.verbosity + '\n')
    if args.mode == 'qarnot':
        f.write('env_variables_to_collect:\n')
        f.write('  - QARNOT_COMPUTE_API_URL\n')
        f.write('  - QARNOT_STORAGE_API_URL\n')

def readparts_from_yaml(file) -> int:
    with open(file, 'r') as configFile:
        content = yaml.safe_load(configFile) 
        try:
            #print(content)
            return content["payloads_nb"]
        except:
            logging.critical('Unknown field')

def create_pollux_yaml(args, pollux_path) -> int:
    if os.path.exists(pollux_path):
        os.remove(pollux_path)
    if args.initial_file:
        if os.path.isfile(args.initial_file):
            logging.info('create \'pollux.yaml\' from %s', args.initial_file)
            shutil.copy2(args.initial_file, pollux_path)
            #read from yaml
            nb_parts = readparts_from_yaml(args.initial_file)
            return nb_parts
        else:
            logging.critical('%s is not a valid path', args.initial_file)
    else:
        create_pollux_configuration(pollux_path, args, args.nb_parts)
        return args.nb_parts

def clean_containers(client):
    for container in client.containers.list():
        if container.name == 'pollux' or container.name.startswith('zebulon'):
            container.remove()
    client.containers.prune()

def clean_networks(client):
    for network in client.networks.list():
        if network.name == 'pollux':
            network.remove()
    client.networks.prune()

def clean_volumes(client):
    for volumes in client.volumes.list():
        if network.name == 'pollux':
            network.remove()
    client.networks.prune()

def clean_docker(client):
   clean_containers(client)
   clean_networks(client)
   clean_volumes(client)

def run_local_mode(args) -> int:
    logging.info('Running local mode')
    pollux_configuration = 'pollux.yaml'
    nb_parts = create_pollux_yaml(args, pollux_configuration)
    #
    if "POLLUX_INSTALL" not in os.environ:
        logging.critical('In local mode, $POLLUX_INSTALL env variable needs to be defined and point to POLLUX installation.') 

    pollux_path = "$POLLUX_INSTALL/pollux"
    pollux_bin = os.path.expandvars(pollux_path)

    #call pollux locally
    subprocess.run(pollux_bin, check=True)

def run_local_docker_mode(args) -> int:
    logging.info('Running local docker mode')
    pollux_path = 'pollux.yaml'
    nb_parts = create_pollux_yaml(args, pollux_path)

    #docker should not be called by pollux master
    #but all docker env should be created here: network...
    import docker

    client = docker.from_env()
    #clean before launching
    clean_docker(client)
    #create volume
    #pollux_volume = client.volumes.create(name='pollux', driver='local')
    #create network
    logging.info("Create docker bridge network 'pollux'")
    client.networks.create("pollux", driver="bridge")
    #create master pollux
    image = args.image
    image += ":" + args.image_tag
    pollux_configuration_path = os.path.abspath('pollux.yaml')
    pollux_volume_bind = {pollux_configuration_path: {'bind': '/pollux/pollux.yaml', 'mode': 'ro'}}
    pollux_ports_bind = {'443/tcp': 443}
    working_dir = '/pollux'
    client.containers.run(image, name='pollux', command='/root/pollux', 
      working_dir=working_dir, network='pollux', volumes=pollux_volume_bind,
      ports=pollux_ports_bind, detach=True)

    #create zebulons
    for i in range(nb_parts):
        name = 'zebulon_' + str(i+1)
        command = './zebulon --id ' + str(i+1)
        command += " --hostname " + name
        command += " --mode localdocker"
        command += " --port 50000"
        command += " --master_ip pollux" 
        command += " --master_port 50000"
        command += " --stdout"
        client.containers.run(image, name=name, command=command, network='pollux', detach=True)

def run_qarnot_mode(args) -> int:
    logging.info('Running qarnot mode')
    import qarnot
    conn = qarnot.Connection('qarnot.conf')

    if args.ssh:
        qarnot_profile = 'docker-cluster-network-ssh'
    else:
        qarnot_profile = 'docker-cluster'
    logging.info('Using %s profile', qarnot_profile)
    logging.info('And %s docker image', args.image)

    if args.task_name != "":
        task_name = args.task_name
    else:
        task_name = args.image
        if args.ssh:
            task_name += '_ssh'
        task_name += '_' + datetime.datetime.now().strftime("%d_%m__%H_%M") 

    pollux_path = 'pollux.yaml'
    nb_parts = create_pollux_yaml(args, pollux_path)

    task = conn.create_task(task_name, qarnot_profile, nb_parts+1)
    #snapshot every 5s
    task.snapshot(5)

    task.constants["DOCKER_REPO"] = args.image
    task.constants["DOCKER_TAG"] = args.image_tag

    if args.clean_buckets:
        try:
            input_bucket = conn.retrieve_bucket(args.input_bucket)
            input_bucket.delete()
        except Exception as e:
            pass
        try:
            output_bucket = conn.retrieve_bucket(args.output_bucket)
            output_bucket.delete()
        except Exception as e:
            pass
    input_bucket = conn.create_bucket(args.input_bucket)


    input_bucket.add_file('pollux.yaml', remote='pollux.yaml')
    #Attach the bucket to the task
    task.resources.append(input_bucket)

    # May be changed to /opt/script_ssh.sh or /opt/script_mpi_hostname.sh
    if args.ssh:
        command =  "/bin/bash -c 'mkdir -p ~/.ssh " \
                   "&& echo \"${DOCKER_SSH}\" >> ~/.ssh/authorized_keys " \
                   "&& /usr/sbin/sshd -D" \
                   "& /root/pollux'"
        task.constants['DOCKER_CMD_MASTER'] = command
        home_dir = os.path.expanduser( '~' )
        id_pub_path = os.path.join(home_dir, '.ssh', 'id_rsa.pub')
        id_pub = open(id_pub_path, "r")
        task.constants["DOCKER_SSH"] = id_pub.read()
        id_pub.close()
    else:
        task.constants['DOCKER_CMD_MASTER'] = "/root/pollux"
    task.constants['DOCKER_CMD_WORKER'] = "/root/zebulon -o qarnot --master_port 50000 --port 50000"
    output_bucket = conn.create_bucket(args.output_bucket)
    task.results = output_bucket

    # Store if an error happened during the process
    error_happened = False
    try:
        logging.info('** Submitting %s...' % task.name)
        task.submit()

        last_state = ''
        ssh_tunneling_done = False
        done = False
        while not done:
            if task.state != last_state:
                last_state = task.state
                logging.info('** {} >>> {}'.format(task.name, last_state))
            done = task.wait(2)

            if args.ssh and not ssh_tunneling_done:
                # Wait for the task to be FullyExecuting
                if task.state == 'FullyExecuting':
                    # If the ssh connexion was not done yet and the list active_forward is available (len!=0)
                    forward_list = task.status.running_instances_info.per_running_instance_info[0].active_forward
                    if len(forward_list) != 0:
                        ssh_forward_port = forward_list[0].forwarder_port
                        ssh_forward_host = forward_list[0].forwarder_host
                        cmd = f"ssh -o StrictHostKeyChecking=no root@{ssh_forward_host} -p {ssh_forward_port}"
                        logging.info('cmd: {}'.format(cmd))
                        ssh_tunneling_done = True


        # Display fresh stdout / stderr
        map(sys.stdout.write, task.fresh_stdout())
        map(sys.stderr.write, task.fresh_stderr())

        if task.state == 'Failure':
            logging.critical('** %s >>> Errors: %s' % (task.name, task.errors[0]))
            error_happened = True

    except Exception as e:
            logging.critical('** %s >>> Errors: %s' % (task.name, e))
            error_happened = True

    finally:
        #task.delete(purge_resources=True, purge_results=True)
        # Exit code in case of error
        if error_happened:
            return 1
        return 0

def main() -> int:
    parser = argparse.ArgumentParser(description='Pollux Qarnot job submitter.')
    parser.add_argument('-m', '--mode', help='Launch mode',
                        choices=['local', 'qarnot', 'local_docker'], action='store', default='local')
    parser.add_argument('-v', '--verbosity', help='Verbosity level',
                        choices=['info', 'debug', 'trace'], action='store', default='info')
    parser.add_argument('-a', '--task_name', help='qarnot task name', action='store', default="")
    parser.add_argument('-i', '--image', help='docker image name', action='store', default='polluxio/pollux-payload-examples')
    parser.add_argument('-t', '--image_tag', help='docker image tag', action='store', default='latest')
    parser.add_argument('-p', '--payload', help='payload path', action='store', default='/root/pollux-payload-test')
    parser.add_argument('-b', '--input_bucket', help='input bucket name', action='store', default='pollux-qarnot-input')
    parser.add_argument('-o', '--output_bucket', help='output bucket name', action='store', default='pollux-qarnot-output')
    parser.add_argument('-s', '--ssh', help='enable ssh access on Qarnot side', action='store_true')
    parser.add_argument('-k', '--clean_buckets',
                        help='(Qarnot mode only): clean input and output buckets if they already exist before creating them.',
                        action='store_true')
    group = parser.add_mutually_exclusive_group()
    group.add_argument('-n', '--nb_parts', help='number of parts', action='store', type=int, default=2)
    group.add_argument('-f', '--initial_file', help='initial yaml file', action='store')

    args = parser.parse_args()

    loggingLevel = logging.INFO
    if args.verbosity != 'info':
        loggingLevel = logging.DEBUG

    logging.basicConfig(
        level=loggingLevel,
        format="%(asctime)s [%(threadName)-12.12s] [%(levelname)-5.5s]  %(message)s",
        handlers=[
            logging.FileHandler("pollux_launcher.log"),
            logging.StreamHandler()
    ])

    if args.mode == 'local':
        run_local_mode(args)
    elif args.mode == 'qarnot':
        run_qarnot_mode(args)
    elif args.mode == 'local_docker':
        run_local_docker_mode(args)


if __name__ == '__main__':
    sys.exit(main())
