pipeline {
  agent { dockerfile true }

  stages {
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
          sh 'ctest -T test --no-compress-output'
        }
      }
    }
  }
  // post {
  //   always {
  //     // Archive the CTest xml output
  //     archiveArtifacts (
  //       artifacts: 'build/Testing/**/*.xml',
  //       fingerprint: true
  //     )

  //     // Process the CTest xml output with the xUnit plugin
  //     xunit (
  //       testTimeMargin: '3000',
  //       thresholdMode: 1,
  //       thresholds: [
  //         skipped(failureThreshold: '0'),
  //         failed(failureThreshold: '0')
  //       ],
  //     tools: [CTest(
  //         pattern: 'build/Testing/**/*.xml',
  //         deleteOutputFiles: true,
  //         failIfNotNew: false,
  //         skipNoTestFiles: true,
  //         stopProcessingIfError: true
  //       )]
  //     )

  //     // Clear the source and build dirs before next run
  //     deleteDir()
  //   }
  // }
}
