import qarnot
import os

conn = qarnot.connection.Connection(os.path.join(os.path.expanduser('~'), 'qarnot.conf'))

task = conn.create_task('Hello World in Python', 'docker-batch', 4)

task.constants['DOCKER_REPO'] = 'ubuntu'
task.constants['DOCKER_TAG'] = 'latest'
task.constants['DOCKER_CMD'] = 'echo Hello World from ${INSTANCE_ID}!'

task.submit()
print(task.stdout())
