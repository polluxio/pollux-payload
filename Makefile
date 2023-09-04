all: build-image tag-image push-image

alpine: build-alpine-image tag-alpine-image push-alpine-image

build-image:
	sudo docker build --target polluxapp --file Dockerfile . -t polluxapp

build-alpine-image:
	sudo docker build --target polluxapp --file Dockerfile.alpine . -t polluxapp-alpine

docker-login:
	sudo docker login

tag-image:
	sudo docker tag polluxapp christophealex/polluxapp

tag-alpine-image:
	sudo docker tag polluxapp-alpine christophealex/polluxapp-alpine

push-alpine-image:
	sudo docker push christophealex/polluxapp-alpine

push-image:
	sudo docker push christophealex/polluxapp
