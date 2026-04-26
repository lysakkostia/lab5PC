from locust import HttpUser, task, between

class WebServerUser(HttpUser):
    wait_time = between(0.1, 1.0)

    @task(3)
    def index_page(self):
        self.client.get("/")

    @task(2)
    def second_page(self):
        self.client.get("/page2.html")

    @task(1)
    def not_found_page(self):
        self.client.get("/random_missing_page.html")