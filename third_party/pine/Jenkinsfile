pipeline {
    agent { 
        docker { 
            image 'nixos/nix' 
            args '-u root --privileged -v $HOME/nix-cache:/nix' 
        } 
    }
    stages {
        stage('build') {
            steps {
                sh '''
                rm -rf reports
                mkdir reports
                nix-channel --add https://nixos.org/channels/nixpkgs-unstable nixpkgs
                nix-channel --update
                cd utils/
                nix-shell --run "cd ../ && meson build && cd build && ninja"
                '''
            }
        }
        // we could probably test the IPC with different emulators here in
        // parallel when that's a thing
        stage('tests') {
            steps {
                sh '''
                cd utils/
                nix-shell --run "../build/tests -r junit -o ../reports/pcsx2.xml"
                '''
            }
        }
        stage('release') {
            steps {
                sh '''
                cd utils/
                nix-shell --run "sh -c ./build-release.sh"
                '''
            }
        }
    }
    post {
        always {
            archiveArtifacts artifacts: 'release.zip', fingerprint: true
            junit 'reports/*.xml'
            publishCoverage adapters: [coberturaAdapter('build/meson-logs/coverage.xml')]
        }
    }
}
