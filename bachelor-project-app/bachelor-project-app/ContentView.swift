//
//  ContentView.swift
//  bachelor-project-app
//
//  Created by David Hansson on 02/04/2024.
//

import SwiftUI

struct ContentView: View {

    let reader = NFCReader()
    let writer = NFCWriter()
    
    @State var writeId = 2
    @State var writeOption = 0
    @FocusState private var isWriteLocationFocused: Bool
    
    var body: some View {
        VStack(spacing: 30) {
            Text("Welcome \(UserManager.name) #\(UserManager.id)")
                .font(.largeTitle)
                .fontWeight(.medium)
                .padding(.vertical, 60)
                .padding(.horizontal)
            
            Button(action: {
                reader.read()
            }, label: {
                Text("Check in scan")
            })
            
            Spacer()
            
            VStack(spacing: 0) {
                Divider()
                
                HStack {
                    Picker("What is your favorite color?", selection: $writeOption) {
                        Text("Location box").tag(0)
                        Text("User card").tag(1)
                    }
                    .pickerStyle(.segmented)
                    .frame(maxWidth: .infinity)
                    
                    Button(action: {
                        if writeOption == 0 {
                            writer.message = "location/\(writeId)"
                        } else {
                            writer.message = "user/\(writeId)"
                        }
                        writer.write()
                    }, label: {
                        Text("Setup with id:")
                    })
                    
                    HStack {
                        Spacer()
                        TextField("", value: $writeId, formatter: NumberFormatter())
                            .keyboardType(.numberPad)
                            .foregroundStyle(Color.blue)
                            .multilineTextAlignment(.center)
                            .focused($isWriteLocationFocused)
                        Spacer()
                    }
                    .frame(width: 50)
                }
                .padding(.horizontal, 2)
            }
        }
        .toolbar {
            ToolbarItem(placement: .keyboard) {
                Button("Done") {
                    isWriteLocationFocused = false
                }
            }
        }
    }
}

#Preview {
    ContentView()
}
