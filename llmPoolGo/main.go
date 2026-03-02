package main

import (
	"bytes"
	"encoding/base64"
	"encoding/json"
	"fmt"
	"io"
	"log"
	"net/http"
	"os"
	"path/filepath"
	"strings"
	"sync"
)

const (
	ollamaURL  = "http://localhost:11434/api/generate"
	modelName  = "qwen3-vl:latest"
	numWorkers = 3
)

type OllamaRequest struct {
	Model  string   `json:"model"`
	Prompt string   `json:"prompt"`
	Stream bool     `json:"stream"`
	Images []string `json:"images"`
}

type OllamaResponse struct {
	Response string `json:"response"`
}

type Task struct {
	ImagePath string
}

func analyzeImage(imagePath string) (string, error) {
	data, err := os.ReadFile(imagePath)
	if err != nil {
		return "", fmt.Errorf("failed to read image: %v", err)
	}

	encoded := base64.StdEncoding.EncodeToString(data)

	reqBody := OllamaRequest{
		Model:  modelName,
		Prompt: "Describe this image in exactly 10 lines. Stay extremely concise. One observation per line.",
		Stream: false,
		Images: []string{encoded},
	}

	jsonBody, err := json.Marshal(reqBody)
	if err != nil {
		return "", fmt.Errorf("failed to marshal request: %v", err)
	}

	resp, err := http.Post(ollamaURL, "application/json", bytes.NewBuffer(jsonBody))
	if err != nil {
		return "", fmt.Errorf("failed to call Ollama: %v", err)
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		body, _ := io.ReadAll(resp.Body)
		return "", fmt.Errorf("ollama returned error (%d): %s", resp.StatusCode, string(body))
	}

	var ollamaResp OllamaResponse
	if err := json.NewDecoder(resp.Body).Decode(&ollamaResp); err != nil {
		return "", fmt.Errorf("failed to decode response: %v", err)
	}

	return ollamaResp.Response, nil
}

func worker(id int, tasks <-chan Task, wg *sync.WaitGroup) {
	defer wg.Done()
	for task := range tasks {
		fmt.Printf("[Worker %d] Processing: %s\n", id, filepath.Base(task.ImagePath))
		result, err := analyzeImage(task.ImagePath)
		if err != nil {
			log.Printf("[Worker %d] Error analyzing %s: %v\n", id, task.ImagePath, err)
			continue
		}

		outputFile := task.ImagePath + ".txt"
		err = os.WriteFile(outputFile, []byte(result), 0644)
		if err != nil {
			log.Printf("[Worker %d] Error writing result for %s: %v\n", id, task.ImagePath, err)
			continue
		}
		fmt.Printf("[Worker %d] Saved result to: %s\n", id, filepath.Base(outputFile))
	}
}

func main() {
	// Root images directory from the C++ project
	imgDir := "../images"

	files, err := os.ReadDir(imgDir)
	if err != nil {
		log.Fatalf("Failed to read images directory: %v", err)
	}

	tasks := make(chan Task, len(files))
	var wg sync.WaitGroup

	// Start workers
	for w := 1; w <= numWorkers; w++ {
		wg.Add(1)
		go worker(w, tasks, &wg)
	}

	// Send tasks
	count := 0
	for _, file := range files {
		if !file.IsDir() && strings.HasSuffix(strings.ToLower(file.Name()), ".jpg") {
			tasks <- Task{ImagePath: filepath.Join(imgDir, file.Name())}
			count++
		}
	}
	close(tasks)

	fmt.Printf("Started processing %d images with %d workers...\n", count, numWorkers)
	wg.Wait()
	fmt.Println("All tasks completed.")
}
