void setBuildStatus(String message, String state) {
  step([
      $class: "GitHubCommitStatusSetter",
      reposSource: [$class: "ManuallyEnteredRepositorySource", url: "https://github.com/jakobod/network-driver"],
      contextSource: [$class: "ManuallyEnteredCommitContextSource", context: "ci/jenkins/build-status"],
      errorHandlers: [[$class: "ChangingBuildStatusErrorHandler", result: "UNSTABLE"]],
      statusResultSource: [ $class: "ConditionalStatusResultSource", results: [[$class: "AnyBuildResult", message: message, state: state]] ]
  ]);
}


pipeline {
  agent { dockerfile { additionalBuildArgs '--no-cache' } }

  stages {
    stage('Init') {
      steps{
        setBuildStatus("Building...", "PENDING");
        sh 'rm -rf build'
      }
    }

    stage('Configure') {
      steps {
        sh './configure --enable-testing'
      }
    }

    stage('Build') {
      steps {
        dir('build') {
          sh 'make'
        }
      }
    }

    stage('Test') {
      steps {
        dir('build') {
          sh 'ctest -T test --no-compress-output --output-on-failure'
        }
      }
    }
  }

  post {
    success {
      setBuildStatus("Build succeeded!", "SUCCESS");
    }
    failure {
      setBuildStatus("Build failed!", "FAILURE");
    }
  }
}
