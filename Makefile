build-image:
	sudo docker build --target polluxapp --file Dockerfile . -t polluxapp

docker-login:
	sudo docker login

tag-image:
	sudo docker tag polluxapp christophealex/polluxapp

push-image:
	sudo docker push christophealex/polluxapp

all: build-image tag-image push-image