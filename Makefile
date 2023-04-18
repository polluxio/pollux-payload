build-image:
	sudo docker build --target polluxapp . -t polluxapp

build-qarnot-image:
	sudo docker build --target polluxapp-qarnot --file Dockerfile.qarnot . -t polluxapp-qarnot

docker-login:
	sudo docker login

tag-image:
	sudo docker tag polluxapp christophealex/polluxapp

tag-qarnot-image:
	sudo docker tag polluxapp-qarnot christophealex/polluxapp-qarnot

push-image:
	sudo docker push christophealex/polluxapp

push-qarnot-image:
	sudo docker push christophealex/polluxapp-qarnot


all: build-image tag-image push-image

qarnot: build-qarnot-image tag-qarnot-image push-qarnot-image
