all: build-image tag-image push-image

alpine: build-alpine-image tag-alpine-image push-alpine-image

build-image:
	sudo docker build --target pollux-payload-examples --file Dockerfile . -t pollux-payload-examples

build-alpine-image:
	sudo docker build --target pollux-payload-examples --file Dockerfile.alpine . -t pollux-payload-alpine

docker-login:
	sudo docker login

tag-image:
	sudo docker tag pollux-payload-examples polluxio/pollux-payload-examples

tag-alpine-image:
	sudo docker tag pollux-payload-examples-alpine polluxio/pollux-payload-examples-alpine

push-alpine-image:
	sudo docker push polluxio/pollux-payload-examples-alpine

push-image:
	sudo docker push polluxio/pollux-payload-examples
