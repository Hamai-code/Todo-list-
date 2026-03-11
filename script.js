const API = "";

const form = document.getElementById("task-form");
const messageEl = document.getElementById("message");
const taskBody = document.getElementById("task-body");
const doneMonthEl = document.getElementById("done-month");

const toLocalDateString = (dateISO) => {
  if (!dateISO) return "";
  const [year, month, day] = dateISO.split("-");
  return `${day}/${month}/${year}`;
};

const currentMonthKey = () => {
  const now = new Date();
  return `${now.getFullYear()}-${String(now.getMonth() + 1).padStart(2, "0")}`;
};

const showMessage = (text, isError = false) => {
  messageEl.textContent = text;
  messageEl.className = `message ${isError ? "error" : "ok"}`;
};

async function charger() {
  const response = await fetch(`${API}/taches`);
  const data = await response.json();

  if (!response.ok) {
    showMessage(data.error || "Impossible de charger les taches", true);
    return;
  }

  taskBody.innerHTML = "";

  data.forEach((task) => {
    const row = document.createElement("tr");
    if (task.etat === 1) row.classList.add("done");

    row.innerHTML = `
      <td>${task.id}</td>
      <td>${task.description}</td>
      <td>${task.date}</td>
      <td><input type="checkbox" ${task.etat === 1 ? "checked" : ""} ${task.etat === 1 ? "disabled" : ""}></td>
      <td class="actions">
        <button class="btn-done">Faite</button>
        <button class="btn-edit">Modifier</button>
        <button class="btn-delete">Supprimer</button>
      </td>
    `;

    row.querySelector(".btn-done").addEventListener("click", async () => {
      await fetch(`${API}/faite/${task.id}`, { method: "POST" });
      await charger();
    });

    row.querySelector(".btn-delete").addEventListener("click", async () => {
      await fetch(`${API}/supprimer/${task.id}`, { method: "DELETE" });
      await charger();
    });

    row.querySelector(".btn-edit").addEventListener("click", async () => {
      const newDesc = window.prompt("Nouvelle description", task.description);
      if (!newDesc) return;

      const proposedDate = task.date_iso || "";
      const newDate = window.prompt("Nouvelle date (YYYY-MM-DD)", proposedDate);
      if (!newDate) return;

      const res = await fetch(`${API}/modifier/${task.id}`, {
        method: "PUT",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ description: newDesc.trim(), date: newDate.trim() }),
      });

      const payload = await res.json();
      if (!res.ok) {
        showMessage(payload.error || "Echec de la modification", true);
      }

      await charger();
    });

    taskBody.appendChild(row);
  });

  const month = currentMonthKey();
  const doneThisMonth = data.filter((task) => task.etat === 1 && task.date_iso?.startsWith(month)).length;
  doneMonthEl.textContent = String(doneThisMonth);
}

form.addEventListener("submit", async (event) => {
  event.preventDefault();

  const description = document.getElementById("desc").value.trim();
  const date = document.getElementById("date").value;

  if (!description || !date) {
    showMessage("Description et date obligatoires", true);
    return;
  }

  const response = await fetch(`${API}/ajout`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ description, date }),
  });

  const payload = await response.json();

  if (!response.ok) {
    showMessage(payload.error || "Echec de l'ajout", true);
    return;
  }

  form.reset();
  showMessage(`Tache ajoutee pour le ${toLocalDateString(date)}`);
  await charger();
});

charger().catch(() => showMessage("Erreur de connexion au serveur C", true));

